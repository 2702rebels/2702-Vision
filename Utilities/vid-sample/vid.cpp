#include <unistd.h>
#include <cstdint>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#include <linux/videodev2.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <drm/drm_fourcc.h>
#include <thread>
#include <condition_variable>

#include <string>
#include <stdexcept>

using namespace std;

#include "v4lcam.h"
#include "fbdisp.h"
#include "cammgr.h"

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

class imgProc : public FrmProcessor
{
public:
	imgProc()
	{
	}
	virtual ~imgProc()
	{

	}
	virtual void ProcessFrame( int nCam, CameraFrame const *pcf )
	{
		printf( "Have Frame from %d\n", nCam);
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
		imgProc proc;
		CameraManager mycam0( "/dev/video0", 0, 640, 480, DRM_FORMAT_RGB888, proc );
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
