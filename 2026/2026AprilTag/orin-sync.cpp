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
static const uint32_t s_u32SECONDARY_RESTART_MESSAGE =	0xA5270205;
static const uint32_t s_u32CAMERA_FRAME_MESSAGE		=	0xA5270206;

inline size_t UnPack( float &f, uint8_t const *pu8, size_t sBufLen = sizeof(float) )
{
	if (sBufLen < sizeof(float)) {
		throw std::runtime_error( "UnPack float - not enough data" );
	}
	memcpy( &f, pu8, sizeof(float) );
	return sizeof(float);
}

inline size_t UnPack( uint16_t &u16, uint8_t const *pu8, size_t sBufLen = sizeof(uint16_t) )
{
	if (sBufLen < sizeof(u16)) {
		throw std::runtime_error( "UnPack u16 - not enough data" );
	}
	memcpy( &u16, pu8, sizeof(u16) );
	return sizeof(u16);
}

inline size_t UnPack( uint32_t &u32, uint8_t const *pu8, size_t sBufLen = sizeof(uint32_t) )
{
	if (sBufLen < sizeof(u32)) {
		throw std::runtime_error( "UnPack u32 - not enough data" );
	}
	memcpy( &u32, pu8, sizeof(u32) );
	return sizeof(u32);
}

inline size_t Pack( float const f, uint8_t *pu8, size_t s )
{
	if (s < sizeof(f)) {
		throw std::runtime_error( "Pack float - out of space"  );
	}
	memcpy( pu8, &f, sizeof(f) );
	return sizeof(f);
}

inline size_t Pack( uint16_t u16, uint8_t *pu8, size_t s )
{
	if (s < sizeof(u16)) {
		throw std::runtime_error( "Pack u16 - out of space"  );
	}
	memcpy( pu8, &u16, sizeof(u16) );
	return sizeof(u16);
}

inline size_t Pack( uint32_t u32, uint8_t *pu8, size_t s )
{
	if (s < sizeof(u32)) {
		throw std::runtime_error( "Pack u32 - out of space"  );
	}
	memcpy( pu8, &u32, sizeof(u32) );
	return sizeof(u32);

}

COrinSync::COrinSync( bool bPrimary, MailBox<ATFrame *>&mbFrame, int nSecondaries ) :
	m_bPrimary( bPrimary ),
	m_mbFrame( mbFrame ),
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

printf( "About to Bind\n" );
	vBind();

printf( "About to start thread\n" );
	m_thSync = std::thread( &COrinSync::vSyncThread, this );
printf( "started thread\n" );

	std::this_thread::yield();
printf( "After yield\n" );
	while( m_mtx.try_lock() ) {
		m_mtx.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

}

COrinSync::~COrinSync()
{
	m_bShutdown = true;

	m_thSync.join();

	if (m_nSock >= 0) {
		close( m_nSock );
	}
}

void COrinSync::vSyncThread()
{
	printf( "vSyncThread()\n" );
	m_mtx.lock();
	printf( "After Lock()\n" );
	if (m_bPrimary) {
		vPrimaryWorker();
	}
	else {
		vSecondaryWorker();
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

// Primary continually waits for packets
void COrinSync::vPrimaryWorker()
{
	printf( "starting Primary Worker\n" );

	enum { WAIT_FOR_SECONDARIES, WAIT_FOR_CAM_START, RUNNING } eState = WAIT_FOR_SECONDARIES;
	// Space for
	//   4 bytes message id
	//   4 bytes Sequence#
	//	 2 bytes cam #
	//	 2 bytes # tags
	//	32 tags
	//      1 byte tag ID
	//		4 corners
	//			2 floats (2 * 4 = 8 bytes);
	uint8_t gru8Buff[1024 + 12];

	struct sockaddr_in saBroadcast;

	saBroadcast.sin_family = AF_INET;
	saBroadcast.sin_port = htons(nSyncPort);
	uint32_t u32Addr = (10 << 24) | ((nTeamNumber / 100) << 16) | ((nTeamNumber % 100) << 8) | 0xFF;
	saBroadcast.sin_addr.s_addr = htonl( u32Addr ); // Broadcast

	try {
		while (!m_bShutdown) {
			struct sockaddr_in saRemote;
			socklen_t sLen = sizeof(saRemote);
			bool bMsgRcvd = false;

			int nRcv = recvfrom( m_nSock, gru8Buff, sizeof(gru8Buff), 0, (struct sockaddr *)&saRemote, &sLen );
			if (nRcv == -1 ){
			}
			else if (nRcv >= sizeof(uint32_t)) {
				uint32_t u32MsgId;
				bMsgRcvd = true;
				uint8_t const *pu8 = gru8Buff;

				pu8 += UnPack( u32MsgId, pu8, sizeof(gru8Buff) );
				printf( "Rcvd: %08x\n", u32MsgId );
				switch ( u32MsgId ) {
				case s_u32SECONDARY_READY_MESSAGE:
					{
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
						printf( "Have %ld of %d 2ndarys\n", m_vSecondaries.size(), m_nSecondaries );
						if (m_vSecondaries.size() == m_nSecondaries) {
							eState = WAIT_FOR_CAM_START;
						}
					}
					break;
				case s_u32CAMERA_STARTED_MESSAGE:
					{
						int nStarted = 0;
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
						// if all secondaries have started their cameras,  signal main thread to start cameras.
						if (nStarted == m_nSecondaries) {
							m_mtx.unlock();
							eState = RUNNING;
						}
					}
					break;
				case s_u32SECONDARY_RESTART_MESSAGE:
					{
						// A secondary has restarted for some reason, we need to do the same...
						// TODO fixme
					}
					break;
				case s_u32CAMERA_FRAME_MESSAGE:
					{
						timeval ts = {};
						uint32_t u32Seq;
						uint16_t u16Cam;
						uint16_t u16Tags;

						pu8 += UnPack( u32Seq, pu8 );
						pu8 += UnPack( u16Cam, pu8 );
						pu8 += UnPack( u16Tags, pu8 );
						ATFrame *patf = new ATFrame( u16Cam, u32Seq, &ts );
						for (uint16_t u16 = 0; u16 < u16Tags; u16++) {
							double grCorners[4][2];
							uint8_t u8Id = *pu8++;
							for (int c = 0; c < 4; c++) {
								float fU;
								float fV;
								pu8 += UnPack( fU, pu8 );
								pu8 += UnPack( fV, pu8 );
								grCorners[c][0] = fU;
								grCorners[c][1] = fV;
							}
							ATDetection atd( u8Id, grCorners );
							patf->vAddDetection( atd );
							m_mbFrame.post( patf );
						}
					}
					break;
				}
			}

			if (!bMsgRcvd) {
				uint32_t u32Msg = 0;
				switch (eState) {
				case WAIT_FOR_SECONDARIES:
					u32Msg = s_u32PRIMARY_READY_MESSAGE;
					break;
				case WAIT_FOR_CAM_START:
					u32Msg = s_u32START_CAMERA_MESSAGE;
					break;
				case RUNNING:
					break;
				}
				if (u32Msg) {
					int nErr = sendto( m_nSock, &u32Msg, sizeof(u32Msg), 0, (struct sockaddr *)&saBroadcast, sizeof(saBroadcast));
					printf( "Sending %08X\n", u32Msg );
				}
			}
		}
	}
	catch (std::exception &e) {
		printf( "PrimaryWorker: %s\n", e.what() );
	}
}

bool COrinSync::bPrimarySync()
{
	m_mtx.lock();
	m_mtx.unlock();
	return true;
}

void COrinSync::vSecondaryWorker()
{
	uint8_t gru8Buff[1024 + 12];
	enum { RESTARTED, WAIT_FOR_CAM, RCVD_CAM_START, CAM_STARTED } eState;

	struct sockaddr_in saBroadcast;

	saBroadcast.sin_family = AF_INET;
	saBroadcast.sin_port = htons(nSyncPort);
	uint32_t u32Addr = (10 << 24) | ((nTeamNumber / 100) << 16) | ((nTeamNumber % 100) << 8) | 0xFF;
	saBroadcast.sin_addr.s_addr = htonl( u32Addr ); // Broadcast

	while (!m_bShutdown) {
		struct sockaddr_in saRemote;
		uint32_t u32Msg;

		socklen_t sLen = sizeof(saRemote);
		bool bMsgRcvd = false;
		int nRcv = recvfrom( m_nSock, gru8Buff, sizeof(gru8Buff), 0, (struct sockaddr *)&saRemote, &sLen );
		if (nRcv < 0 ) {
		}
		else if (nRcv >= sizeof(uint32_t)) {
			bMsgRcvd = true;
			UnPack( u32Msg, gru8Buff, sizeof(gru8Buff) );
//			printf( "Received %08x\n", u32Msg );
			switch( u32Msg ) {
			case s_u32PRIMARY_READY_MESSAGE:
				{
				printf( "Received PRIMARY_READY_MESSAGE\n" );
					m_bCamStartSent = false;
					eState = WAIT_FOR_CAM;
					if (!m_vSecondaries.size()) {
						m_vSecondaries.push_back( saRemote );
					}
					int nErr = sendto( m_nSock, &s_u32SECONDARY_READY_MESSAGE, sizeof(s_u32SECONDARY_READY_MESSAGE), 0, (struct sockaddr *)&saRemote, sLen);
				}
				break;
			case s_u32START_CAMERA_MESSAGE:
				printf( "Received START_CAMERA_MESSAGE\n" );
				// on the first START_CAMERA_MESSAGE, switch state and unlock the mutex so the main thread
				// can start the camera and call bSendCamStarted();
				if (eState == WAIT_FOR_CAM) {
					eState = RCVD_CAM_START;
					m_mtx.unlock();
				}
				// if Camera Started has been sent, primary probably didn't receive, so send again
				else if (m_bCamStartSent) {
					int nErr = sendto( m_nSock, &s_u32CAMERA_STARTED_MESSAGE, sizeof(s_u32CAMERA_STARTED_MESSAGE), 0, (struct sockaddr *)&saRemote, sLen);
				}
				break;
			}
		}

		if (!bMsgRcvd && eState == RESTARTED) {
			int nErr = sendto( m_nSock, &s_u32SECONDARY_RESTART_MESSAGE, sizeof(s_u32CAMERA_STARTED_MESSAGE), 0, (struct sockaddr *)&saBroadcast, sizeof(saBroadcast));
		}
	}
}

bool COrinSync::bSecondarySync()
{
	m_mtx.lock();
	m_mtx.unlock();
	return true;
}

bool COrinSync::bSendCamStarted()
{
	if (m_vSecondaries.size() == 0) {
		throw std::runtime_error( "OrinSync::bSendCamStarted() Primary not set\n" );
	}
	
	int nErr = sendto( m_nSock, &s_u32CAMERA_STARTED_MESSAGE, sizeof(s_u32CAMERA_STARTED_MESSAGE), 0, (struct sockaddr *)&m_vSecondaries[0].GetSockaddr(), sizeof(struct sockaddr_in));
	
	return true;
}

bool COrinSync::bSendFrame( ATFrame *patf )
{
	uint8_t gru8Buff[1024];
	uint8_t *pu8 = gru8Buff;
	uint8_t const *pu8End = pu8 + sizeof(gru8Buff);
	
	uint32_t u32Seq = patf->u32Seq();
	uint16_t u16Cam = patf->nCam();
	uint16_t u16Tags = patf->sTags();

	pu8 += Pack( s_u32CAMERA_FRAME_MESSAGE, pu8, pu8End - pu8 );
	pu8 += Pack( u32Seq, pu8, pu8End - pu8 );
	pu8 += Pack( u16Cam, pu8, pu8End - pu8 );
	pu8 += Pack( u16Tags, pu8, pu8End - pu8 );

	for (uint16_t u16 = 0; u16 < u16Tags; u16++ ) {
		ATDetection const &atd = (*patf)[u16];
		*pu8++ = atd.uId();
		for (int i = 0; i < 4; i++) {
			cv::Point2f const &c = atd[i];
			float fU = c.x;
			float fV = c.y;
			pu8 += Pack( fU, pu8, pu8End - pu8 );
			pu8 += Pack( fV, pu8, pu8End - pu8 );
		}
	}

//	printf( "Sending Frame: Tags = %d size = %ld\n", u16Tags, pu8 - gru8Buff );
	int nErr = sendto( m_nSock, gru8Buff, pu8 - gru8Buff, 0, (struct sockaddr *)&m_vSecondaries[0].GetSockaddr(), sizeof(struct sockaddr_in));

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
	try {
		MailBox<ATFrame *> mb;
		COrinSync os( true, mb, 1 );
		os.bPrimarySync();
	}
	catch( std::exception &e ) {
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
