#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <span>
#include <iostream>
#include <ifaddrs.h>
#include <string>
#include <stdexcept>

#include "timesync.h"
#include "packunpack.h"

// Time synchronization with RIO

static const int nSyncPort = 27020;
static const int nRIOAddr = 2;

#define CLOCK_TO_USE	CLOCK_REALTIME
//#define CLOCK_TO_USE	CLOCK_MONOTONIC_RAW
class  TSPing
{
	uint8_t		m_u8ProtoVers;
	uint8_t		m_u8MsgId;
	uint64_t	m_u64ClntTime;
public:
	TSPing( uint64_t u64Time ) :
		m_u8ProtoVers( 1 ),
		m_u8MsgId( 1 ),
		m_u64ClntTime( u64Time )
	{
	}
	~TSPing()
	{
	}

	inline size_t Pack( uint8_t *pu8, size_t sLen )
	{
		size_t sIdx = 0;

		sIdx += ::Pack( m_u8ProtoVers,  pu8 + sIdx, sLen - sIdx );
		sIdx += ::Pack( m_u8MsgId,      pu8 + sIdx, sLen - sIdx );
		sIdx += ::Pack( m_u64ClntTime, pu8 + sIdx, sLen - sIdx );

		return sIdx;
	}

	inline uint64_t u64ClntTime() const
	{
		return m_u64ClntTime;
	}
};

class TSPong
{
	uint8_t		m_u8ProtoVers;
	uint8_t		m_u8MsgId;
	uint64_t	m_u64ClntTime;
	uint64_t	m_u64SrvrTime;
	uint64_t	m_u64EpochTime;
	uint8_t		m_u8MatchType;
	uint8_t		m_u8MatchNum;
public:
	TSPong( uint8_t const *pu8, size_t sLen )
	{
		size_t sIdx = 0;
		
		sIdx += UnPack( m_u8ProtoVers, pu8 + sIdx, sLen - sIdx );
		sIdx += UnPack( m_u8MsgId,     pu8 + sIdx, sLen - sIdx );
		sIdx += UnPack( m_u64ClntTime, pu8 + sIdx, sLen - sIdx );
		sIdx += UnPack( m_u64SrvrTime, pu8 + sIdx, sLen - sIdx );
		sIdx += UnPack( m_u64EpochTime, pu8 + sIdx, sLen - sIdx );
		sIdx += UnPack( m_u8MatchType, pu8 + sIdx, sLen - sIdx );
		sIdx += UnPack( m_u8MatchNum,  pu8 + sIdx, sLen - sIdx );
	}
	~TSPong()
	{
	}
	inline uint64_t u64ClntTime() const
	{
		return m_u64ClntTime;
	}
	inline uint64_t u64SrvrTime() const
	{
		return m_u64SrvrTime;
	}
	inline uint64_t u64EpochTime() const
	{
		return m_u64EpochTime;
	}
	inline uint8_t u8MatchType() const
	{
		return m_u8MatchType;
	}
	inline uint8_t u8MatchNum() const
	{
		return m_u8MatchNum;
	}
};

CTimeSync::CTimeSync( int nTeamNum, int nPollMS ) :
	m_nTeamNum( nTeamNum ),
	m_nPollMS( nPollMS )
{
	m_u32RIOAddr = (10 << 24) | ((m_nTeamNum / 100) << 16) | ((m_nTeamNum % 100) << 8) | 2;

	m_nSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_nSock < 0) {
		throw std::runtime_error( "Unable to create Socket" );
	}

	// Set up socket to timeout at requested interval
	struct timeval tv;
	tv.tv_sec = nPollMS / 1000; 
	tv.tv_usec = (nPollMS % 1000) * 1000;
	
	if (setsockopt(m_nSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
		throw std::runtime_error( "CTimeSync:: failed setting SO_RCVTIMEO" );
	}

	vBind();

	m_thSync = std::thread( &CTimeSync::vSyncThread, this );
}

CTimeSync::~CTimeSync()
{
	m_bShutdown = true;

	m_thSync.join();

	if (m_nSock >= 0) {
		close( m_nSock );
	}
}

void CTimeSync::vSyncThread()
{
	uint8_t gru8Pong[sizeof(TSPong)];

	try {
		while (!m_bShutdown) {
			struct sockaddr_in saRemote;
			socklen_t sLen = sizeof(saRemote);

			// Send a Ping
			vSendPing();

			// receive Pongs until we get a timeout
			bool bRcvd;
			do {
				bRcvd = false;

				// Wait for a pong.
				int nRcv = recvfrom( m_nSock, gru8Pong, sizeof(gru8Pong), 0, (struct sockaddr *)&saRemote, &sLen );
				if (nRcv == -1 ){
				}
				else {
					vHandlePong( gru8Pong, nRcv );
					bRcvd = true;
				}
			} while( bRcvd );
		}
	}
	catch (std::exception &e) {
		printf( "vSyncThread(): Exception %s\n", e.what() );
	}
}

void CTimeSync::vBind()
{
	struct ifaddrs *ifap, *ifa;
	if (getifaddrs(&ifap) == -1) {
		throw std::runtime_error( "Unable to get Network Interfaces" );
	}

	
	for (ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in const *pAddr = (struct sockaddr_in const *)ifa->ifa_addr;
			uint32_t u32Addr = ntohl( pAddr->sin_addr.s_addr );

			if ((u32Addr & 0xFFFFFF00) == (m_u32RIOAddr & 0xFFFFFF00)) {
				sockaddr_in sa = *pAddr;
				sa.sin_port = htons(nSyncPort);
				sa.sin_addr.s_addr = htonl(INADDR_ANY);
				int nErr = bind( m_nSock, (struct sockaddr *)&sa, sizeof(sa) );
				break;
			}
		}
	}
	freeifaddrs(ifap); // Free the allocated memory
}

void CTimeSync::vSendPing()
{
	struct sockaddr_in saddr;

	memset( &saddr, 0, sizeof(saddr) );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(nSyncPort);
	saddr.sin_addr.s_addr = htonl( m_u32RIOAddr );

	uint8_t	gru8Buff[sizeof(TSPing)];

	struct timespec ts;
	clock_gettime( CLOCK_TO_USE, &ts );
	uint64_t u64ClntTime = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;

	TSPing tpi( u64ClntTime );
	
	size_t sIdx = tpi.Pack( gru8Buff, sizeof(gru8Buff) );

	if (sendto(m_nSock, gru8Buff, sIdx, 0, (const struct sockaddr *)&saddr, sizeof(saddr)) != sIdx ) {
		printf( "vSendPing(): sendto failed\n" );
	}
}

void CTimeSync::vHandlePong( uint8_t const *pu8, size_t sLen )
{
	timespec ts;
	clock_gettime( CLOCK_TO_USE, &ts );
	uint64_t u64CurTime = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;

	TSPong tpo( pu8, sLen );

	unsigned uRoundTrip = u64CurTime - tpo.u64ClntTime();
	unsigned uOneWay = uRoundTrip / 2;
	// Calculate the difference between our time and the RIO time assuming that
	// the round trip network time is the same both directions.
	m_i64Offset = tpo.u64SrvrTime() - (u64CurTime - uOneWay) + m_i64WPILibCorrection;
	m_u8MatchType = tpo.u8MatchType();
	m_u8MatchNum = tpo.u8MatchNum();

#if defined(TEST)
	printf( "Offset = %ld\n", m_i64Offset );
#endif

	// if the epoch time is more than 1 day ahead of our current time, try to
	// adjust our stytem time.
	if (tpo.u64EpochTime() > (ts.tv_sec + 24 * 3600)) {

		clock_gettime( CLOCK_TO_USE, &ts );
		// calculate the delta between current time and new time
		uint64_t u64Delta = (tpo.u64EpochTime() - ts.tv_sec);

		ts.tv_sec = tpo.u64EpochTime();

		// if the clock is successfully updated, adjust the
		// offset by the difference between old time and new
		if (clock_settime( CLOCK_REALTIME, &ts ) == 0) {
//			printf( "Clock Delta = %lu\n", u64Delta );
			m_i64WPILibCorrection = u64Delta * 1000000LL;
			m_i64Offset += m_i64WPILibCorrection;
		}
#ifdef TEST
		else {
			printf( "Tried to set date but failed\n" );
		}
#endif
	}
	
}


#ifdef TEST
int main( int argc, char **argv )
{
	try {
		CTimeSync ts( 2702 );
		
		printf( "Running for 60 seconds...\n" );
		std::this_thread::sleep_for( std::chrono::seconds(60) );
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}
	return 0;
}
#endif
#ifdef TEST_SECONDARY
int main( int argc, char **argv )
{
	try {
		MailBox<ATFrame *> mb;
		COrinSync os( false, mb, 0 );
		os.bSecondarySync();
printf( "Cameras should start...\n" );
		os.bSendCamStarted();
	}
	catch( std::exception &e ) {
		printf( "Exception: %s\n", e.what() );
	}
	return 0;
}

#endif
