#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <X11/Xlib.h>
//#include <cameraserver/CameraServer.h>
//#include <networktables/NetworkTableInstance.h>

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

class imgProc : public nvFrmProcessor
{
	std::mutex	m_mtxCS;
	bool		m_bFirstVid = true;
	cs::CvSource 	m_outputStream;
public:
	imgProc()
	{
	}
	virtual ~imgProc()
	{

	}
	virtual void ProcessFrame( int nCam,  const nvFrame *pcf )
	{
		printf( "Have Frame from %d:%d - %d/%d\n", nCam, pcf->u32Sequence(), pcf->nCols(), pcf->nRows() );
		if (m_mtxCS.try_lock()) { 
			cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC1, (void *)pcf->pu8Image() );
			if (m_bFirstVid) {
				m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows() );
				m_bFirstVid = false;
			}
			m_outputStream.PutFrame(frame);
			m_mtxCS.unlock();
		}
	}
};

int main( int argc, char **argv )
{
	XInitThreads();
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
