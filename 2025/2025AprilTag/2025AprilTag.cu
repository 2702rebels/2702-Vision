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

#include <signal.h>

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

class imgProc : public nvFrmProcessor
{
	std::mutex	m_mtxAT;
	bool		m_bFirstAT = true;

	std::mutex	m_mtxCS;
	bool		m_bFirstVid = true;
	cs::CvSource 	m_outputStream;

    detectorstruct detector;
public:
	imgProc()
	{
	}
	virtual ~imgProc()
	{

	}
	virtual void ProcessFrame( int nCam,  const nvFrame *pcf )
	{
//		printf( "Have Frame from %d:%d - %d/%d\n", nCam, pcf->u32Sequence(), pcf->nCols(), pcf->nRows() );
		cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC1, (void *)pcf->pu8Image() );
		if (m_mtxAT.try_lock()) {
			if (m_bFirstAT) {
				detector.camera_matrix = create_camera_matrix();
				detector.dist_coeffs = create_distortion_coefficients();
				detector.apriltag_detector_ = maketagdetector(tag36h11_create());
				detector.gpu_detector_ = 
					new frc971::apriltag::GpuDetector(pcf->nCols(), pcf->nRows(), 
					 detector.apriltag_detector_, 
					 detector.camera_matrix, detector.dist_coeffs);
				m_bFirstAT = false;
			}
			
	        	detector.gpu_detector_->DetectGray((uint8_t *)pcf->pu8Image());
#if 0
       		 	const zarray_t *detections = detector.gpu_detector_->Detections();
		        if( zarray_size(detections)) {
				for (int i = 0; i < zarray_size(detections); ++i) {
					 //std::cout << "971 library has " << zarray_size(detections) << " detections" << std::endl;
					 apriltag_detection_t *gpu_detection;
					 zarray_get(detections, i, &gpu_detection);
					 std::cout << "tag: " << gpu_detection->id << " at " <<
							 gpu_detection->c[0] << "," << gpu_detection->c[1]
							 << std::endl;
				}
			}
#endif
			m_mtxAT.unlock();
		}
		else {
			printf( "Skipping %d\n", pcf->u32Sequence() );
		}
		if (m_mtxCS.try_lock()) { 
			if (m_bFirstVid) {
				m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows() );
				m_bFirstVid = false;
			}
//			m_outputStream.PutFrame(frame);
			m_mtxCS.unlock();
		}
	}
};

int main( int argc, char **argv )
{
	signal( SIGINT, sighandler );

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
		imgProc proc;
//		nvCamera mycam0( "/dev/video0", 0, 1280, 800, V4L2_PIX_FMT_RGB24, proc );
		nvCamera mycam0( "/dev/video0", 0, 1280, 800, V4L2_PIX_FMT_GREY, proc );
	//	nvCamera mycam1( "/dev/video1", 4, 1 );

		printinfo( mycam0 );
		while( !bStop ) {
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	return 0;
}
