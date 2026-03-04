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

#include "ApriltagReport.h"

#include "syncgen.h"

#define NUM_CAMERAS	2

static bool grbUseTag[23] =
{
	false,		// 0
	false,		// 1
	false,		// 2
	false,		// 3
	false,		// 4
	false,		// 5
	true,		// 6
	true,		// 7
	true,		// 8
	true,		// 9
	true,		// 10
	true,		// 11
	false,		// 12
	false,		// 13
	false,		// 14
	false,		// 15
	false,		// 16
	true,		// 17
	true,		// 18
	true,		// 19
	true,		// 20
	true,		// 21
	true,		// 22
};

using ApriltagReportStructType = wpi::Struct<ApriltagReport>; // Structure class that will contain the serialization and deserialization functions for the object detection packet
#define MAX_EXP	0x18000

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
	if (nCam == 0) {
		return frc971::apriltag::CameraMatrix{ 1139.15, 628.77, 1137.01, 338.60 };
	}
	else {
		return frc971::apriltag::CameraMatrix{ 1114.92, 655.70, 1112.39, 331.96 };
	}
}

frc971::apriltag::DistCoeffs create_distortion_coefficients( int nCam ) {
	if (nCam == 0) {
		return frc971::apriltag::DistCoeffs{ -0.453151, 0.209991, -0.000833, 0.000511, 0 };
	}
	else {
		return frc971::apriltag::DistCoeffs{ -0.437922, 0.204096, 0.001379, 0.001860, 0 };
	}
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

Field g_field;
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
#if 1
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
		if (m_mtxAT.try_lock()) {
			if (m_bFirstAT) {
				pCam->bSetGain( 500 );
				pCam->bSetExposure( 100000 );
				pCam->bSetGain( 250 );
				pCam->bSetExposure( 10000 );
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
				if (nId >= 1 && nId <= 22 && grbUseTag[nId] ) {
					ATDetection atd( gpu_detection->id, gpu_detection->p );
					patf->vAddDetection( atd );
				}
			}

			detector.gpu_detector_->ReinitializeDetections();
			m_mb.post( patf );

			m_mtxAT.unlock();
		}
		else {
			printf( "Skipping %d:%d\n", pCam->nCam(), pcf->u32Sequence() );
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
	printf( "Usage: 2025AprilTag [-s]\n" );
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
		i64TimeOffset /= 1000000;
		time_t tCur = -i64TimeOffset;

		printf( "%s\n", ctime( &tCur) );

	}
	else {
		printf( "tscb::: -> %x\n", e.flags );
	}
}

bool bSolvePose( int nCam, cv::Mat &objPoints, cv::Mat &imgPoints, CUDPSender &udpSend, timeval const &ts  )
{
	bool bHaveSolution = false;

	cv::Mat rvec( 3, 1, CV_64F );
	cv::Mat tvec( 3, 1, CV_64F );

	if (solvePnP( objPoints, imgPoints, camMatrix( nCam ), camDistortion( nCam ), rvec, tvec )) {
		cv::Mat reprojectPoints( objPoints.rows, 1, CV_32FC2);
		cv::projectPoints(objPoints, rvec, tvec, camMatrix(nCam), camDistortion(nCam), reprojectPoints);

		float sum = 0.;
		sum = cv::norm(reprojectPoints, imgPoints);

		Rebels::Point3d p3Rvec( rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2) );
		Rebels::Point3d p3Tvec( tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2) );
		Rebels::Quaternion qRot( p3Rvec );

		bool bUseSolve = sum < 10.;

		if (bUseSolve) {
			// Run through the apriltag corners transformed to "camera coordinates".
			// If there are 2 tags in view, make sure that all corners are within 14ft of the camera.
			// If there is only a single tag in view, make sure that corners are within 60 inches
			float fMaxDist = (objPoints.rows == 4) ? 60.F : 14.F*12.F;
			for (int i = 0; i < objPoints.rows; i++) {
				Rebels::Point3d p3Crnr( objPoints.at<float>(i * 3), objPoints.at<float>(i * 3 + 1), objPoints.at<float>(i * 3 + 2) );
				p3Crnr = qRot.RotatePoint(p3Crnr) + p3Tvec;

//printf( "%.3f  %.3f  %.3f\n", p3Crnr.x(), p3Crnr.y(), p3Crnr.z() );
				if (p3Crnr.z() > fMaxDist) {
					bUseSolve = false;
				}
			}
		}

		if (bUseSolve)
		{
			if (nCam == 0) {
				p3Tvec += g_p3dCam0;
			}
			else {
				p3Tvec += g_p3dCam1;
			}

			qRot.Inverse();

			Rebels::Point3d p3dField = qRot.RotatePoint( p3Tvec );
			p3dField *= -1.;
			
			Rebels::Point3d p3dV1( 0.0, 0.0, 1.0 );
			Rebels::Point3d p3dV2 = qRot.RotatePoint( p3dV1 );
			Rebels::Point3d p3dX( 1.0, 0.0, 0.0 );

			
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
			int64_t time = nt::Now() + i64TimeOffset - tvDelta.tv_usec; // timestamp is at frame capture time in microseconds with NetworkTables server-adjusted time

			ApriltagReport ar( 0, time );

			ar.vSetPose( 1, p3dField.x(), p3dField.y(), atan2( p3dV2.y(), p3dV2.x() ), 0. );

			uint8_t gru8Packet[ApriltagReportStructType::GetSize()];
			std::span<uint8_t> sPacket(gru8Packet, sizeof(gru8Packet));
			ApriltagReportStructType::Pack(sPacket, ar );
			udpSend.bSend( sPacket );
//printf( "Cam %d -> sending solution %.3f %.3f %.3f\n", nCam, p3dField.x(), p3dField.y(), atan2( p3dV2.y(), p3dV2.x() ) * 180 / 3.14159 );
			bHaveSolution = true;
		}
	}

	return bHaveSolution;
}

class CFrameLogger
{
	FILE 		*m_fp = 0;
public:
	CFrameLogger()
	{
	}
	~CFrameLogger()
	{
		if (m_fp) {
			fclose( m_fp );
			m_fp = 0;
		}
	}

	bool bOpenLog( char const *psz )
	{
		m_fp = fopen( psz, "wt" );
		if (!m_fp) {
			fprintf( stderr, "Unable to open log file '%s'\n", psz );
			return false;
		}
		return true;
	}

	void vLogFrame( std::vector<ATFrame> const &vaf )
	{
		if (m_fp) {
			fprintf( m_fp, "S:%u\n", vaf[0].u32Seq() );
			fprintf( m_fp, "Cams:%lu\n", vaf.size() );
			for( size_t s = 0; s < vaf.size(); s++ ) {
				auto f = vaf[s];
				fprintf( m_fp, "T:%lu\n", f.sTags() );
				// Loop through Tags
				for (size_t sT = 0; sT < f.sTags(); sT++) {
					ATDetection const &atd = f[sT];
					fprintf( m_fp, "%d", atd.uId() );
					for( int i = 0; i < 4; i++ ) {
						fprintf( m_fp, ",%8.3f,%8.3f", atd[i].x, atd[i].y );
					}
					fprintf( m_fp, "\n" );
				}
			}
		}
	}
};

Rebels::MultiCam mc_cam;

int main( int argc, char **argv )
{
	SyncGen							sg;
	nt::BooleanPublisher			bpLogging;
	nt::BooleanSubscriber			bsIsLogging;
	bool bLogStarted = false;
	int nSyncFreq = -1;
	char const *pszLog = 0;
	char const *pszCamFile = 0;
	CFrameLogger	cfl;

	bool bNetTableServer = false;

	signal( SIGINT, sighandler );

	for (int n = 1; n < argc; n++) {
		if (argv[n][0] == '-') {
			switch (argv[n][1]) {
			case 'f':
				nSyncFreq = atoi( argv[n] + 2 );
//				printf( "Freq = %d\n", nSyncFreq );
				break;
			case 's':
				bNetTableServer = true;
				break;
			case 'l':
				pszLog = argv[n] + 2;
				cfl.bOpenLog( pszLog );
				break;
			case 'c':
				pszCamFile = argv[n] + 2;
				break;
			default:
				Usage();
				break;
			}
		}
	}

	if( !pszCamFile ) {
		Usage();
		return 1;
	}

	MailBox<ATFrame *>	mbFrame;

	g_field = Field( "2026-rebuilt-welded.json" );
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
		CUDPSender udpSend;

	//	auto ntinst = nt::NetworkTableInstance::GetDefault();
	//	ntinst.StartClientTeam( 2702 );
	//	ntinst.StartServer();
	//		RPICamera mycam0( "/dev/video0", 0, 800, 600, DRM_FORMAT_BGR888, proc );
	//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		imgProc proc0(mbFrame);
		imgProc proc1(mbFrame);
		nvCamera mycam0( "/dev/video0", 0, 1280, 800, V4L2_PIX_FMT_GREY, proc0 );
		nvCamera mycam1( "/dev/video1", 1, 1280, 800, V4L2_PIX_FMT_GREY, proc1 );

		if (nSyncFreq > 0) {
			std::this_thread::sleep_for( std::chrono::milliseconds(100) );
			if( !sg.bConnect( "/dev/ttyUSB0" )) {
				printf( "Unable to connect to SyncGenerator\n" );
				return 1;
			}
			sg.vStop();
			sg.vStart( nSyncFreq );
		}
//		mycam0.bSetExposure( 10000 );
//		mycam1.bSetExposure( 10000 );
//		mycam0.bSetGain( 1000 );
//		mycam1.bSetGain( 1000 );

		printinfo( mycam0 );

		ATFrame *grpFrms[NUM_CAMERAS] = { 0 };

		while( !bStop ) {
			ATFrame *patf;

			if (mbFrame.waitfor( patf, 100 )) {
				int nCam = patf->nCam();

				delete grpFrms[nCam];
				grpFrms[nCam] = patf;

				bool bHaveAllCams = false;
				// If Frame from Camera 0 is valid, see if frames from
				// all other cameras are valid and match the sequence number
				if (grpFrms[0]) {
					uint32_t u32Seq = grpFrms[0]->u32Seq();

					bHaveAllCams = true;
					for (int i = 1; i < NUM_CAMERAS; i++) {
						if (!grpFrms[i] || grpFrms[i]->u32Seq() != u32Seq) {
							bHaveAllCams = false;
							break;
						}
					}
				}

				if (bHaveAllCams) {
					std::vector<ATFrame> vfrms;

					for (int nCam = 0; nCam < NUM_CAMERAS; nCam++ ) {
						vfrms.push_back( *grpFrms[nCam] );
					}
					bool bHaveSolution = false;
					for (int nCam = 0; !bHaveSolution && nCam < NUM_CAMERAS; nCam++) {
						int nTags = grpFrms[nCam]->sTags();
						if (nTags >= 2) {
							cv::Mat objPoints(4 * nTags, 1, CV_32FC3);
							cv::Mat imgPoints(4 * nTags, 1, CV_32FC2);

							int idx = 0;
							for (size_t t = 0; t < grpFrms[nCam]->sTags(); t++ )
							{
								ATDetection const &d = (*grpFrms[nCam])[t];
								Rebels::QXfrm const &xfm = g_field[d.uId() - 1];
//printf( "Have Detection %d\n", d.uId() );
								for (int c = 0; c < 4; c++, idx++ ) {
									Rebels::Point3d pn = xfm.TransformPoint(grCorners[c]);
//									Rebels::Point3d pn = grCorners[c];
									objPoints.ptr<cv::Vec3f>(0)[idx] = cv::Vec3f(pn.x(), pn.y(), pn.z() );
									imgPoints.ptr<cv::Point2f>(0)[idx] = d[c];
								}
							}

							bHaveSolution = bSolvePose( nCam, objPoints, imgPoints, udpSend, grpFrms[nCam]->ts() );
						}
					}
					for (int nCam = 0; !bHaveSolution && nCam < 2; nCam++) {
						int nTags = grpFrms[nCam]->sTags();
						if (nTags >= 1) {
							cv::Mat objPoints(4 * nTags, 1, CV_32FC3);
							cv::Mat imgPoints(4 * nTags, 1, CV_32FC2);

							int idx = 0;
							for (size_t t = 0; t < grpFrms[nCam]->sTags(); t++ )
							{
								ATDetection const &d = (*grpFrms[nCam])[t];
								Rebels::QXfrm const &xfm = g_field[d.uId() - 1];
								for (int c = 0; c < 4; c++, idx++ ) {
									Rebels::Point3d pn = xfm.TransformPoint( grCorners[c] );
//									Rebels::Point3d pn = grCorners[c];
									objPoints.ptr<cv::Vec3f>(0)[idx] = cv::Vec3f(pn.x(), pn.y(), pn.z() );
									imgPoints.ptr<cv::Point2f>(0)[idx] = d[c];
								}
							}

							bHaveSolution = bSolvePose( nCam, objPoints, imgPoints, udpSend, grpFrms[nCam]->ts() );
						}
					}
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
