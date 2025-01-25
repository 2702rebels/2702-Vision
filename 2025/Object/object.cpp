#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

//#include <X11/Xlib.h>
//#include <cameraserver/CameraServer.h>
//#include <networktables/NetworkTableInstance.h>
#include <drm/drm_fourcc.h>

#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <string>
#include <stdexcept>

#include "rpicam.h"

using namespace std;

#include <signal.h>

#include "object.h"

bool bStop = false;

void sighandler( int nSig )
{
	printf( "Received Signal %d\n", nSig );
	bStop = true;
}

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
		imgProc proc;
//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		RPICamera mycam0( "/dev/video0", 0, 800, 600, DRM_FORMAT_BGR888, proc );
	//	nvCamera mycam1( "/dev/video1", 4, 1 );

	//	printinfo( mycam0 );
		while( !bStop ) {
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	return 0;
}
