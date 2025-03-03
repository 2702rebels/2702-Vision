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
#include <opencv2/highgui.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <string>
#include <stdexcept>

#include <cameraserver/CameraServer.h>
#include <networktables/NetworkTableInstance.h>
#include <drm/drm_fourcc.h>

using namespace std;

#include "v4lcam.h"
#include "fbdisp.h"
#include "nvcam.h"

#include "mbox.h"

#include <signal.h>

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
frc971::apriltag::CameraMatrix create_camera_matrix() {
  return frc971::apriltag::CameraMatrix{
      1,
      1,
      1,
      1,
  };
}

frc971::apriltag::DistCoeffs create_distortion_coefficients() {
  return frc971::apriltag::DistCoeffs{
      0, 0, 0, 0, 0,
  };
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

class ATDetection
{
	unsigned			m_uId;
	vector<cv::Point>	m_vxCorners;
public:
	ATDetection( unsigned uId, double grCrnrs[4][2] )
	{
		m_uId = uId;
		for( int i = 0; i < 4; i++ )
		{
			m_vxCorners.push_back( cv::Point( grCrnrs[i][0], grCrnrs[i][1] ) );
		}
	}
	ATDetection( const ATDetection &src ) :
		m_uId( src.m_uId ),
		m_vxCorners( src.m_vxCorners )
	{
	}
	~ATDetection()
	{
	}
	unsigned uId() const
	{
		return m_uId;
	}

	ATDetection &operator=( ATDetection const &src )
	{
		m_uId = src.m_uId;
		m_vxCorners = src.m_vxCorners;

		return *this;
	}

	cv::Point const &operator[](int n) const
	{
		return m_vxCorners[n];
	}
};

class ATFrame
{
	int					m_nCam;
	uint32_t			m_u32Seq;
	struct timeval		m_ts;
	vector<ATDetection>	m_vxTags;
public:
	ATFrame( int nCam, uint32_t u32Seq, timeval const *ts ) :
		m_nCam( nCam ),
		m_u32Seq( u32Seq )
	{
		m_ts.tv_sec = ts->tv_sec;
		m_ts.tv_usec = ts->tv_usec;
	}
	~ATFrame()
	{
	}

	int	nCam() const 
	{
		return m_nCam;
	}

	uint32_t u32Seq() const
	{
		return m_u32Seq;
	}

	timeval const &ts() const
	{
		return m_ts;
	}

	size_t sTags() const
	{
		return m_vxTags.size();
	}

	ATDetection const &operator[]( int n ) const
	{
		return m_vxTags[n];
	}

	void vAddDetection( ATDetection &atd )
	{
		m_vxTags.push_back( atd );
	}
};

class imgProc : public nvFrmProcessor
{
	std::mutex	m_mtxAT;
	bool		m_bFirstAT = true;

	std::mutex	m_mtxCS;
	bool		m_bFirstVid = true;
	cs::CvSource 	m_outputStream;

	detectorstruct detector;

	MailBox<ATFrame *>	&m_mb;

public:
	imgProc( MailBox<ATFrame *> &mb ) :
		m_mb( mb )
	{
	}
	virtual ~imgProc()
	{
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
				m_outputStream.PutFrame(frame);
				m_mtxCS.unlock();
			}
			return;
		}
		if (m_mtxAT.try_lock()) {
			if (m_bFirstAT) {
				pCam->bSetExposure( 10000 );
				pCam->bSetGain( 250 );
				detector.camera_matrix = create_camera_matrix();
				detector.dist_coeffs = create_distortion_coefficients();
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
//				std::cout << "tag: " << gpu_detection->id << " at " << gpu_detection->c[0] << "," << gpu_detection->c[1] << std::endl;
				ATDetection atd( gpu_detection->id, gpu_detection->p );
				patf->vAddDetection( atd );
			}

			m_mb.post( patf );

			m_mtxAT.unlock();
		}
		else {
			printf( "Skipping %d:%d\n", pCam->nCam(), pcf->u32Sequence() );
		}
	}
};

int main( int argc, char **argv )
{
	signal( SIGINT, sighandler );

	MailBox<ATFrame *>	mbFrame;

#if 0
	char szImgName[100];
	sprintf( szImgName, "Testing" );
	cv::namedWindow( szImgName, cv::WINDOW_AUTOSIZE );
	cv::Mat img = cv::imread("img.png", cv::IMREAD_COLOR);
	printf( "img size: %d x %d\n", img.rows, img.cols );
	cv::imshow( szImgName, img );
	cv::waitKey(0);
#endif
	try {
	//	auto ntinst = nt::NetworkTableInstance::GetDefault();
	//	ntinst.StartClientTeam( 2702 );
	//	ntinst.StartServer();
	//		RPICamera mycam0( "/dev/video0", 0, 800, 600, DRM_FORMAT_BGR888, proc );
	//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		imgProc proc0(mbFrame);
		imgProc proc1(mbFrame);
		nvCamera mycam0( "/dev/video0", 0, 1280, 800, V4L2_PIX_FMT_GREY, proc0 );
		nvCamera mycam1( "/dev/video1", 1, 1280, 800, V4L2_PIX_FMT_GREY, proc1 );
//		mycam0.bSetExposure( 10000 );
//		mycam1.bSetExposure( 10000 );
//		mycam0.bSetGain( 1000 );
//		mycam1.bSetGain( 1000 );

		printinfo( mycam0 );
		ATFrame *grpFrms[2] = { 0, 0 };
		while( !bStop ) {
			ATFrame *patf;

			if (mbFrame.wait( patf )) {
				int nCam = patf->nCam();

				delete grpFrms[nCam];
				grpFrms[nCam] = patf;

				if (grpFrms[0] && grpFrms[1]) {
					if( grpFrms[0]->u32Seq() == grpFrms[1]->u32Seq()) {
						printf( "Have Matching Frames at %u\n", patf->u32Seq() );
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
