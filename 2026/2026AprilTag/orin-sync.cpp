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

#include "orin-sync.h"

// Set up Synchronization between cameras on multiple ORINs
//
// Overview:
//  Primary (sync generator)
//		Sends message PRIMARY READY message every 100ms until all secondaries reply with SECONDARY_READY
//		Once all Secondaries have replied with SECONDARY_READY
//		Sends message START CAMERA message every 100ms until all secondaries reply with CAMERA_STARTED
//	

static const int nSyncPort = 27025;
static const int nTeamNumber = 2702;

static const uint32_t s_u32PRIMARY_READY_MESSAGE	=	0xA5270201;
static const uint32_t s_u32SECONDARY_READY_MESSAGE	=	0xA5270202;
static const uint32_t s_u32START_CAMERA_MESSAGE		=	0xA5270203;
static const uint32_t s_u32CAMERA_STARTED_MESSAGE	=	0xA5270204;


COrinSync::COrinSync( bool bPrimary, int nSecondaries ) :
	m_bPrimary( bPrimary ),
	m_nSecondaries( nSecondaries )
{
	m_nSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_nSock < 0) {
		throw std::runtime_error( "Unable to create Socket" );
	}

	// Set up socket for 0.1 second timeout on receive
	struct timeval tv;
	tv.tv_sec = 0; 
	tv.tv_usec = 100000; // .1 second timeout
	

	if (setsockopt(m_nSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
		throw std::runtime_error( "COrinSync:: failed setting SO_RCVTIMEO" );
	}

	// 2. Enable broadcast
	// On some systems, the option can be a char instead of an int
	int broadcastPermission = 1; // 1 = true
	if (setsockopt(m_nSock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0) {
		throw std::runtime_error( "COrinSync:: failed setting SO_BROADCAST" );
	}

	vBind();
}

COrinSync::~COrinSync()
{
	if (m_nSock >= 0) {
		close( m_nSock );
	}
}

void COrinSync::vBind()
{
	struct ifaddrs *ifap, *ifa;
	if (getifaddrs(&ifap) == -1) {
		throw std::runtime_error( "Unable to get Network Interfaces" );
	}

	
	for (ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in const *pAddr = (struct sockaddr_in const *)ifa->ifa_addr;
			uint32_t u32Addr = ntohl( pAddr->sin_addr.s_addr );
			printf( "%08x\n", u32Addr );
			if ((u32Addr & 0xFFFFFF00) == ((10 << 24) | ((nTeamNumber / 100) << 16) | ((nTeamNumber % 100) << 8)) ) {
				sockaddr_in sa = *pAddr;
				sa.sin_port = htons(nSyncPort);
				sa.sin_addr.s_addr = htonl(INADDR_ANY);
				int nErr = bind( m_nSock, (struct sockaddr *)&sa, sizeof(sa) );
				printf( "Bind Returned %d\n", nErr );
				break;
			}
		}
	}
	freeifaddrs(ifap); // Free the allocated memory
}

bool COrinSync::bPrimarySync()
{
	struct sockaddr_in saBroadcast;

	saBroadcast.sin_family = AF_INET;
	saBroadcast.sin_port = htons(nSyncPort);
	uint32_t u32Addr = (10 << 24) | ((nTeamNumber / 100) << 16) | ((nTeamNumber % 100) << 8) | 0xFF;
	saBroadcast.sin_addr.s_addr = htonl( u32Addr ); // Broadcast

	while (m_vSecondaries.size() < m_nSecondaries) {
		int nErr = sendto( m_nSock, &s_u32PRIMARY_READY_MESSAGE, sizeof(s_u32PRIMARY_READY_MESSAGE), 0, (struct sockaddr *)&saBroadcast, sizeof(saBroadcast));
		
		struct sockaddr_in saRemote;
		uint32_t u32Msg;

		bool bMsgRcvd;
		do {
			bMsgRcvd = false;
			socklen_t sLen = sizeof(saRemote);
			int nRcv = recvfrom( m_nSock, &u32Msg, sizeof(u32Msg), 0, (struct sockaddr *)&saRemote, &sLen );
			if (nRcv == sizeof(u32Msg)) {
				bMsgRcvd = true;
				if( u32Msg == s_u32SECONDARY_READY_MESSAGE) {
					printf( "Have 2 rdy\n" );
					bool bNew = true;
					for( auto &s : m_vSecondaries ) {
						if (s.GetSockaddr().sin_addr.s_addr == saRemote.sin_addr.s_addr) {
							bNew = false;
							break;
						}
					}
					if (bNew) {
						m_vSecondaries.push_back(saRemote);
					}
				}
			}
			else {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
				}
				else {
					throw std::runtime_error( "Failed receiving from Secondary" );
				}
			}
		} while (bMsgRcvd );
	}

	// At this point all secondaries have reported in and are ready so start broadcasting the Start Cameras Message
	int nStarted = 0;
	while (nStarted < m_vSecondaries.size()) {
		int nErr = sendto( m_nSock, &s_u32START_CAMERA_MESSAGE, sizeof(s_u32START_CAMERA_MESSAGE), 0, (struct sockaddr *)&saBroadcast, sizeof(saBroadcast));
		
		nStarted = 0;
		struct sockaddr_in saRemote;
		uint32_t u32Msg;

		bool bMsgRcvd;
		do {
			bMsgRcvd = false;
			socklen_t sLen = sizeof(saRemote);
			int nRcv = recvfrom( m_nSock, &u32Msg, sizeof(u32Msg), 0, (struct sockaddr *)&saRemote, &sLen );
			if (nRcv == sizeof(u32Msg)) {
				bMsgRcvd = true;
				if( u32Msg == s_u32CAMERA_STARTED_MESSAGE ) {
					printf( "Have 2 cam\n" );
					for( auto &s : m_vSecondaries ) {
						if (s.GetSockaddr().sin_addr.s_addr == saRemote.sin_addr.s_addr) {
							s.bCamStarted() = true;
							nStarted++;
						}
						else if (s.bCamStarted()) {
							nStarted++;
						}
					}
				}
			}
			else {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
				}
				else {
					throw std::runtime_error( "Failed receiving from Secondary" );
				}
			}
		} while( bMsgRcvd );
	}
	return true;
}

bool COrinSync::bSecondarySync()
{
	while (1) {
		struct sockaddr_in saRemote;
		uint32_t u32Msg;

		socklen_t sLen = sizeof(saRemote);
		int nRcv = recvfrom( m_nSock, &u32Msg, sizeof(u32Msg), 0, (struct sockaddr *)&saRemote, &sLen );
		if (nRcv == sizeof(u32Msg)) {
			if( u32Msg == s_u32PRIMARY_READY_MESSAGE ) {
				if (!m_vSecondaries.size()) {
					m_vSecondaries.push_back( saRemote );
				}
				int nErr = sendto( m_nSock, &s_u32SECONDARY_READY_MESSAGE, sizeof(s_u32SECONDARY_READY_MESSAGE), 0, (struct sockaddr *)&saRemote, sLen);
			}
			else if( u32Msg == s_u32START_CAMERA_MESSAGE ) {
				// once the START_CAMERA Message is received, return tot he main pgm.  It is responsible for starting the camera and calling

				return true;
			}
		}
		else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
			}
			else {
				throw std::runtime_error( "Failed receiving from Secondary" );
			}
		}
	}
}

bool COrinSync::bSendCamStarted()
{
	if (m_vSecondaries.size() == 0) {
		throw std::runtime_error( "OrinSync::bSendCamStarted() Primary not set\n" );
	}
	
	int nErr = sendto( m_nSock, &s_u32CAMERA_STARTED_MESSAGE, sizeof(s_u32CAMERA_STARTED_MESSAGE), 0, (struct sockaddr *)&m_vSecondaries[0].GetSockaddr(), sizeof(struct sockaddr_in));
	
	return true;
}

bool COrinSync::bSend( std::span<uint8_t> const &spBuf )
{
	struct sockaddr_in saddr;

	memset( &saddr, 0, sizeof(saddr) );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(nSyncPort);
	saddr.sin_addr.s_addr = htonl( 0x0A1B0202 );

	sendto(m_nSock, spBuf.data(), spBuf.size(), 0, (const struct sockaddr *)&saddr, sizeof(saddr));

	return true;
}

#ifdef TEST_PRIMARY
int main( int argc, char **argv )
{
	COrinSync os( true, 1 );
	os.bPrimarySync();
	return 0;
}
#endif
#ifdef TEST_SECONDARY
int main( int argc, char **argv )
{
	COrinSync os( false );
	os.bSecondarySync();

	os.bSendCamStarted();
	return 0;
}

#endif
