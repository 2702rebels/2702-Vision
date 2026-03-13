#include <iostream>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

//#include <X11/Xlib.h>

#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <string>
#include <stdexcept>

#include <cameraserver/CameraServer.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/BooleanTopic.h>
#include <networktables/NetworkTableListener.h>
#include <drm/drm_fourcc.h>

using namespace std;

#include "2702math.h"
#include "2702camera.h"
#include "v4lcam.h"
#include "fbdisp.h"
#include "nvcam.h"

#include "mbox.h"

#include <signal.h>

#include "ATFrame.h"
#include "field.h"
#include "udpsender.h"
#include "cfgfile.h"
#include "PoseSolver.h"

#include "ApriltagReport.h"
#include "FrameLogger.h"

#include "syncgen.h"

Rebels::MultiCam mcCam;
Field g_field;
ConfigFile cf;

using ApriltagReportStructType = wpi::Struct<ApriltagReport>; // Structure class that will contain the serialization and deserialization functions for the object detection packet

#define MAX_EXP	0x18000

uint32_t g_u32Gain = 45;
uint32_t g_u32Exposure = 1000;

void printinfo( v4lcam &cam )
{
	cam.printCaps();
	cam.printStandards();
	cam.printControls();
	printf( "---------------\n" );
}

bool bStop = false;

void sighandler( int nSig )
{
	printf( "Received Signal %d\n", nSig );
	bStop = true;
}

//#include "frc971/orin/971apriltag.h"
#include "apriltag_gpu.h"
#include "tag36h11.h"
//#include "third_party/apriltag/apriltag.h"
//#include "third_party/apriltag/tag36h11.h"
frc971::apriltag::CameraMatrix create_camera_matrix( int nCam ) {
	char szTmp[20];

	sprintf( szTmp, "sensor%d", nCam );
	std::string str;

	if (!cf.bGetSetting( szTmp, str )) {
		fprintf( stderr, "Unable to get setting '%s'\n", szTmp );
		exit(1);
	}
	int nSen = atoi( str.c_str() );

	CamParm const &cp = mcCam[nSen];

	printf( "%.4f %.4f %.4f %.4f\n", cp.fFU(), cp.fU0(), cp.fFV(), cp.fV0() );
	return frc971::apriltag::CameraMatrix{ cp.fFU(), cp.fU0(), cp.fFV(), cp.fV0() };
}

frc971::apriltag::DistCoeffs create_distortion_coefficients( int nCam ) {
	char szTmp[20];

	sprintf( szTmp, "sensor%d", nCam );
	std::string str;

	if (!cf.bGetSetting( szTmp, str )) {
		fprintf( stderr, "Unable to get setting '%s'\n", szTmp );
		exit(1);
	}
	int nSen = atoi( str.c_str() );

	CamParm const &cp = mcCam[nSen];

	return frc971::apriltag::DistCoeffs{ cp.dist(0), cp.dist(1), cp.dist(2), cp.dist(3), 0.F };
}

cv::Mat camMatrix( int nCam )
{
	cv::Mat m( 3, 3, CV_32F );
	if (nCam == 0) {
		float grfMat[3][3] = {
			{ 1139.15, 0.00, 628.77 },
			{ 0.00, 1137.01, 338.60 },
			{ 0, 0, 1 }
		};
		for (int r = 0; r < 3; r++ ) {
			for (int c = 0; c < 3; c++ ) {
				m.at<float>( r, c ) = grfMat[r][c];
			}
		}
	}
	else {
		float grfMat[3][3] = {
			{ 1114.92,    0.00, 655.70 },
			{    0.00, 1112.39, 331.96 },
			{    0.00,    0.00,   1 }
		};
		for (int r = 0; r < 3; r++ ) {
			for (int c = 0; c < 3; c++ ) {
				m.at<float>( r, c ) = grfMat[r][c];
			}
		}
	}
	return m;
}

cv::Mat camDistortion( int nCam )
{
	cv::Mat md( 4, 1, CV_64F );
	if (nCam == 0) {
		double grdMat[] = { -0.453151, 0.209991, -0.000833, 0.000511 };

		for (int i = 0; i < 4; i++ )
			md.at<double>(i) = grdMat[i];
	}
	else {
		double grdMat[] = { -0.437922, 0.204096,  0.001379, 0.001860 };
		for (int i = 0; i < 4; i++ )
			md.at<double>(i) = grdMat[i];
	}

	return md;
}

// makes a tag detector.
apriltag_detector_t *maketagdetector(apriltag_family_t *tag_family) {
  apriltag_detector_t *tag_detector = apriltag_detector_create();

  apriltag_detector_add_family_bits(tag_detector, tag_family, 1);

  tag_detector->nthreads = 6;
  tag_detector->wp = workerpool_create(tag_detector->nthreads);
  tag_detector->qtp.min_white_black_diff = 5;
  tag_detector->debug = false;

  return tag_detector;
}

struct detectorstruct {
	frc971::apriltag::GpuDetector *gpu_detector_; 
	apriltag_detector_t *apriltag_detector_;
	frc971::apriltag::CameraMatrix camera_matrix;
	frc971::apriltag::DistCoeffs dist_coeffs;
};

#define LOG_DIR		"/data/logs"

class imgProc : public nvFrmProcessor
{
	std::mutex	m_mtxAT;
	bool		m_bFirstAT = true;

	std::mutex	m_mtxCS;
	bool		m_bFirstVid = true;
	cs::CvSource 	m_outputStream;

	detectorstruct detector;

	MailBox<ATFrame *>	&m_mb;

	std::mutex		m_mtxLog;
	VisionLog		m_vl;

	bool			m_bIsLogging = false;
	bool			m_bLoggingDone = false;
	time_t			m_tmLogEnd;

public:
	imgProc( MailBox<ATFrame *> &mb ) :
		m_mb( mb )
	{
	}
	virtual ~imgProc()
	{
	}
	void vStartLogging()
	{
		m_bIsLogging = true;
	}
	virtual void ProcessFrame( nvCamera *pCam,  const nvFrame *pcf )
	{
//		printf( "Have Frame from %d:%d - %d/%d\n", pCam->nCam(), pcf->u32Sequence(), pcf->nCols(), pcf->nRows() );
		cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC1, (void *)pcf->pu8Image() );
#if 0
		uint64_t u64Exp = pCam->u64GetExposure() + 1;
		u64Exp &= 0xFFFFF;

		pCam->bSetExposure( u64Exp );
#endif
		// The highest priority is to compute the Apriltag as we want this to have the lowest latency possible.
		// The GPU apriltag library is not thread safe, so try to acquire the mutex... if it is busy, simply skip
		// processing this frame.
		if (m_mtxAT.try_lock()) {
			if (m_bFirstAT) {
				pCam->bSetGain( 50 );
				pCam->bSetExposure( 1001 );
				pCam->bSetGain( g_u32Gain );
				pCam->bSetExposure( g_u32Exposure );
				detector.camera_matrix = create_camera_matrix( pCam->nCam() );
				detector.dist_coeffs = create_distortion_coefficients( pCam->nCam() );
				detector.apriltag_detector_ = maketagdetector(tag36h11_create());
				detector.gpu_detector_ = 
					new frc971::apriltag::GpuDetector(pcf->nCols(), pcf->nRows(), 
					 detector.apriltag_detector_, 
					 detector.camera_matrix, detector.dist_coeffs);
				m_bFirstAT = false;
			}
			
			ATFrame *patf = new ATFrame( pCam->nCam(), pcf->u32Sequence(), pcf->ts() );
			detector.gpu_detector_->DetectGray((uint8_t *)pcf->pu8Image());

			const zarray_t *detections = detector.gpu_detector_->Detections();

			for (int i = 0; i < zarray_size(detections); ++i) {
				//std::cout << "971 library has " << zarray_size(detections) << " detections" << std::endl;
				apriltag_detection_t *gpu_detection;
				zarray_get(detections, i, &gpu_detection);
				int nId = gpu_detection->id;
//				if (nId >= 1 && nId <= 22 && grbUseTag[nId] )
				if (1) 
				{
					ATDetection atd( gpu_detection->id, gpu_detection->p );
					patf->vAddDetection( atd );
#if 1
					for (int c = 0; c < 4; c++) {
						cv::circle( frame, atd[c], c * 4, cv::Scalar( 0 ) );
					}
#endif
				}
			}

			detector.gpu_detector_->ReinitializeDetections();
			m_mb.post( patf );

			m_mtxAT.unlock();
		}
		else {
			printf( "Skipping %d:%d\n", pCam->nCam(), pcf->u32Sequence() );
		}

		// We only log and send every 2nd frame to the camera server
		if (pcf->u32Sequence() % 2) {
			if (m_mtxLog.try_lock()) {
				if (m_bLoggingDone) {
				}
				else if( m_vl.bIsOpen()) {
					m_vl.bWriteRaw( frame, pcf->u32Sequence(), pcf->u32Exposure() );
					if (time(0) > m_tmLogEnd) {
						m_vl.vClose();
						m_bLoggingDone = true;
						printf( "Finished Logging\n" );
					}
				}
				else {
					bool bStart = m_bIsLogging;
	//				printf( "Logging = %d\n", bStart);
	//				bool bStart = false;

					if( bStart ) {
						try {
							char szTmp[128];
							for (int i = 0; ; i++ )
							{
								snprintf( szTmp, sizeof(szTmp), "%s/cam%d-%04d.log", LOG_DIR, pCam->nCam(), i );
								if (access( szTmp, F_OK ) == -1) 
									break;
							}
							printf( "Opening log file at %s\n", szTmp );
							m_vl.vOpen( szTmp, true );
							m_tmLogEnd = time(0) + 160;
						}
						catch( exception &e ) {
							m_bLoggingDone = true;
							printf( "Unable to start logging: %s\n", e.what() );
						}
					}
				}		
				m_mtxLog.unlock();
			}
			// Send the frame to the camera server...
			if (m_mtxCS.try_lock()) { 
				if (m_bFirstVid) {
					char szTmp[64];
					sprintf( szTmp, "Apriltag_%d\n", pCam->nCam() );
					m_outputStream = frc::CameraServer::PutVideo( szTmp, pcf->nCols(), pcf->nRows() );
					m_bFirstVid = false;
					printf( "Cam %d: OutputStream set\n", pCam->nCam() );
				}
				
	#if 0
				char szTmp[128];
				sprintf( szTmp, "Exp: %lu", u64Exp );
				cv::putText( frame, szTmp, cv::Point( 10, 20 ), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar( 255, 0, 0) );
	#endif
	#if 0
				uint8_t *pu8 = (uint8_t *)pcf->pu8Image();
				for( int r = 0; r < 10; r++ ) {
					for( int c = 0; c < 640; c++ ) {
						pu8[r * 1280 + c] = 0;
					}
				}
				for( int r = 799; r > 789; r-- ) {
					for( int c = 0; c < 640; c++ ) {
						pu8[r * 1280 + c] = 0;
					}
				}
	#endif
				m_outputStream.PutFrame(frame);
				m_mtxCS.unlock();
			}
			return;
		}
	}
};

#if 0
Rebels::Point3d grCorners[] =
{
	{ -3.25,  3.25, 0. },
	{  3.25,  3.25, 0. },
	{  3.25, -3.25, 0. },
	{ -3.25, -3.25, 0. }
};
#else
Rebels::Point3d grCorners[] =
{
	{ 0., -3.25, -3.25 },
	{ 0.,  3.25, -3.25 },
	{ 0.,  3.25,  3.25 },
	{ 0., -3.25,  3.25 }
};
#endif

void Usage()
{
	printf( "Usage: 2025AprilTag cfgfile [-s]\n" );
	printf( "    -s            run as NetTables Server\n" );
	exit(1);
}

Rebels::Point3d g_p3dCam0( -13.125, 0.00, 8.75 );
Rebels::Point3d g_p3dCam1(  13.125, 0.00, 8.75 );

#include <ntcore_cpp.h>

int64_t i64TimeOffset = 0;

void tscb( nt::Event const &e )
{
	if (e.flags & NT_EVENT_TIMESYNC) {
		printf( "tscb::: Have TIMESYNC\n" );
		nt::TimeSyncEventData const *ptsed = e.GetTimeSyncEventData();
		i64TimeOffset = ptsed->serverTimeOffset;
		printf( "Got Offset -> %ld\n", ptsed->serverTimeOffset );
//		i64TimeOffset /= 1000000;
		time_t tCur = -i64TimeOffset;

		printf( "%s\n", ctime( &tCur) );

	}
	else {
		printf( "tscb::: -> %x\n", e.flags );
	}
}

void vSendFrame( CUDPSender &udpSend, QXfrm const &qxfm, struct timeval const &ts )
		
{
	Rebels::Point3d p3dX( 1.0, 0.0, 0.0 );
	Rebels::Point3d p3dV2 = qxfm.Rotation().RotatePoint( p3dX );

	double fDot = p3dX.DotProduct( p3dV2 );

	struct timespec tsCur;
	struct timeval tvCur;
	struct timeval tvDelta;

		// Get current time since power on
	clock_gettime(CLOCK_MONOTONIC_RAW, &tsCur );
	TIMESPEC_TO_TIMEVAL( &tvCur, &tsCur );
	// Calculate delta between when frame was catured and now.
	timersub( &tvCur, &ts, &tvDelta );
//							auto serverOffset = nt::GetServerTimeOffset( nt::GetDefaultInstance() );
//							if (serverOffset) {
//								i64TimeOffset = serverOffset.value();
//							}
//printf( "now: %lld    Offset: %ld   delta: %u\n", nt::Now(), i64TimeOffset, tvDelta.tv_usec );
	int64_t time = nt::Now() + i64TimeOffset - tvDelta.tv_usec; // timestamp is at frame capture time in microseconds with NetworkTables server-adjusted time

	ApriltagReport ar( 0, time );

	ar.vSetPose( 1, qxfm.Translation().x(), qxfm.Translation().y(), atan2( p3dV2.y(), p3dV2.x() ), 0. );

	uint8_t gru8Packet[ApriltagReportStructType::GetSize()];
	std::span<uint8_t> sPacket(gru8Packet, sizeof(gru8Packet));
	ApriltagReportStructType::Pack(sPacket, ar );
	udpSend.bSend( sPacket );
//printf( "-> sending solution %.3f %.3f %.3f\n", p3dField.x(), p3dField.y(), atan2( p3dV2.y(), p3dV2.x() ) * 180 / 3.14159 );
//printf( "-> sending solution\n" );
}

int main( int argc, char **argv )
{
	bool							bSendToRio = false;
	SyncGen							sg;
	nt::BooleanPublisher			bpLogging;
	nt::BooleanSubscriber			bsIsLogging;
	bool bLogStarted = false;
	int nSyncFreq = -1;
	char const *pszLog = 0;
	CFrameLogger	fl;
	int nCamMode = 0;
	int nSyncMode = 0;

	bool bNetTableServer = false;

	signal( SIGINT, sighandler );

	if (argc < 2) {
		Usage();
		return 1;
	}
	for (int n = 2; n < argc; n++) {
		if (argv[n][0] == '-') {
			switch (argv[n][1]) {
			case 's':
				bNetTableServer = true;
				break;
			case 'l':
				pszLog = argv[n] + 2;
				fl.bCreateLog( pszLog );
				break;
			default:
				Usage();
				break;
			}
		}
	}

	if (!cf.bLoad( argv[1] )) {
		return 1;
	}
	MailBox<ATFrame *>	mbFrame;

	try {
		g_field = Field( cf["FieldFile"].c_str() );
		g_field.vSetIgnore( cf["IgnoreTags"] );
		if( !mcCam.bLoad( cf["camfile"].c_str() ) ) {
			return 1;
		}

		g_u32Gain = atoi( cf["Gain"].c_str() );
		g_u32Exposure = atoi( cf["Exposure"].c_str() );
		nSyncFreq = atoi( cf["FrameFreq"].c_str() );
		nSyncMode = atoi( cf["SyncMode"].c_str() );
		if (nSyncFreq > 0) {
			nCamMode = 3;
		}
		bSendToRio = atoi( cf["SendToRio"].c_str() ) ? true : false;
		std::string strSyncModule;
		if (cf.bGetSetting("SyncModule", strSyncModule)) {
			if( !sg.bConnect( strSyncModule.c_str() )) {
				printf( "Unable to connect to SyncGenerator at '%s'\n", strSyncModule.c_str() );
				return 1;
			}
		}
	}
	catch( std::exception &e ) {
		fprintf( stderr, "Error: %s\n", e.what() );
		return 1;
	}
	for( int i = 0; i < g_field.nTags(); i++ ) {
		Rebels::QXfrm const &q = g_field[i];
		
		printf( "%d: %f\n", int(g_field[i]), q.Translation().x() );
	}

	// start NetworkTables
	auto ntinst = nt::NetworkTableInstance::GetDefault();
	if (bNetTableServer) {
    	ntinst.StartServer();
		ntinst.SetServerTeam(2702);
	}
	else {
		fmt::print("Setting up NetworkTables client for team 2702\n");
		ntinst.StartClient4("2702Vision");
		ntinst.SetServerTeam(2702);
		ntinst.StartDSClient();

		printf( "Waiting for Network Tables to connect\n" );
		int nCount = 0;
		while( !ntinst.IsConnected() && !bStop ) {
			if (nCount++ >2) {
				exit(1);
			}
			printf( "." ); fflush( stdout );
			std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
		}
	}

	nt::NetworkTableListener	ntl = nt::NetworkTableListener::CreateTimeSyncListener( ntinst, true, tscb );
	nt::PubSubOptions pso = 
	{
		.structSize = sizeof(nt::PubSubOptions),
		.pollStorage = 0,
		.periodic = 0.1,
		.excludePublisher = 0,
		.sendAll = true,
		.topicsOnly = false,
		.keepDuplicates = true,
		.prefixMatch = false,
		.disableRemote = false,
		.disableLocal = false,
		.excludeSelf = false
//		.hidden = false
	};


	bpLogging = ntinst.GetBooleanTopic("/Vision/PiIsLogging").Publish( pso );
	bsIsLogging = ntinst.GetBooleanTopic("/Vision/IsLogging").Subscribe(false);
	bpLogging.Set( false );

	try {
		PoseSolver ps( g_field, mcCam );
		CUDPSender udpSend;

		// stop sync generation.  This can always be done as the SyncGen class won't try to send
		// if it hasn't been connected to a SyncModule Arduino
		sg.vStop();

		imgProc proc0(mbFrame);
		imgProc proc1(mbFrame);
		nvCamera mycam0( "/dev/video0", 0, 1280, 800, V4L2_PIX_FMT_GREY, nCamMode, proc0 );
		nvCamera mycam1( "/dev/video1", 1, 1280, 800, V4L2_PIX_FMT_GREY, nCamMode, proc1 );

		// If we are setting up the SyncGenerator,
		// stop the frame clocks.
		if (nSyncFreq > 0) {
			std::this_thread::yield();
			std::this_thread::sleep_for( std::chrono::milliseconds(5) );
			sg.vStart( nSyncFreq );
		}
//		mycam0.bSetExposure( 10000 );
//		mycam1.bSetExposure( 10000 );
//		mycam0.bSetGain( 1000 );
//		mycam1.bSetGain( 1000 );

		printinfo( mycam0 );

		std::vector<ATFrame *> vCamFrames;
		for (int i = 0; i < mcCam.nCam(); i++) {
			vCamFrames.push_back( 0 );
		}

		QXfrm qxRobot;
		bool bHavePose = false;
		while( !bStop ) {
			ATFrame *patf;

			if (mbFrame.waitfor( patf, 100 )) {
				int nCam = patf->nCam();

				// delete previous frame
				delete vCamFrames[nCam];
				vCamFrames[nCam] = patf;

				bool bHaveAllCams = false;
				// If Frame from Camera 0 is valid, see if frames from
				// all other cameras are valid and match the sequence number
				if (vCamFrames[0]) {
					uint32_t u32Seq = vCamFrames[0]->u32Seq();

					bHaveAllCams = true;
					for (int i = 1; i < mcCam.nCam(); i++) {
						if (!vCamFrames[i] || vCamFrames[i]->u32Seq() != u32Seq) {
							bHaveAllCams = false;
							break;
						}
					}
				}

				if (bHaveAllCams && !(vCamFrames[0]->u32Seq() % 4))  {
#if 1
#if 0
					if (bHavePose || ps.bEstimate( qxRobot, vCamFrames )) {
						if( ps.bSolve( qxRobot, vCamFrames ) ) {
							Quaternion const &q = qxRobot.Rotation();
							Point3d const &p = qxRobot.Translation();
							printf( "(%.4f %.4f %.4f %.4f) : (%.3f %.3f %.3f)\n",
								q.q0(), q.x(), q.y(), q.z(), p.x(), p.y(), p.z() );

							bHavePose = true;
							if (bSendToRio) {
								vSendFrame( udpSend, qxRobot, vCamFrames[0]->ts() );
							}
						}
						else {
							bHavePose = false;
						}
					}
#else
					if (ps.bEstimate( qxRobot, vCamFrames )) {
						if (bSendToRio) {
							vSendFrame( udpSend, qxRobot, vCamFrames[0]->ts() );
						}
						else {
							printf( "(%.4f %.4f %.4f %.4f) (%.1f %.1f %.1f)\n",
								qxRobot.Rotation().q0(), qxRobot.Rotation().x(),
								qxRobot.Rotation().y(), qxRobot.Rotation().z(),
								qxRobot.Translation().x(), qxRobot.Translation().y(),
								qxRobot.Translation().z() );
						}
					}
#endif
#endif
					fl.vLogFrame( vCamFrames );
				}
				if (!bLogStarted) {
					auto bLog = bsIsLogging.Get();
					if (bLog) {
						bLogStarted = true;
						proc0.vStartLogging();
						proc1.vStartLogging();
					}							
				}
			}
			
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	return 0;
}
