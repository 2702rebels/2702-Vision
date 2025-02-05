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
#include <networktables/NetworkTableInstance.h>
#include <networktables/NetworkTable.h>
#include <networktables/DoubleTopic.h>

using namespace std;

#include <signal.h>

bool bStop = false;

#include "object.h"
#include "config.h"

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
		NetworkTableInstance networkTableManager = nt::NetworkTableInstance::GetDefault();
		NetworkTable objectDetectionTable = networkTableManager.GetTable("ObjectDetection");
		DoubleTopic hueMinTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/HueMin").Subscribe(0.0);
		DoubleTopic hueMaxTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/HueMax").Subscribe(0.0);
		DoubleTopic saturationMinTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/SaturationMin").Subscribe(0.0);
		DoubleTopic saturationMaxTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/SaturationMax").Subscribe(0.0);
		DoubleTopic valueMinTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/ValueMin").Subscribe(0.0);
		DoubleTopic valueMaxTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/ValueMax").Subscribe(0.0);
		networkTableManager.StartClient4("networkTableClient");
		networkTableManager.SetServerTeam(2702);
		#if false
		networkTableManager.StartDSClient(); // Should only be run on actual robot
		#endif
		imgProc proc;
//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		RPICamera mycam0( "/dev/video0", 0, 800, 600, DRM_FORMAT_BGR888, proc );
	//	nvCamera mycam1( "/dev/video1", 4, 1 );

	//	printinfo( mycam0 );
	ConfigUtils::triggerConfigLoad(proc);
		while( !bStop ) {
			if (ConfigUtils::configHasChanged(proc))
			{
				ConfigUtils::triggerConfigSave(proc);
			}
			if (cv::waitKey(1) == 'l')
			{
				ConfigUtils::triggerConfigLoad(proc);
			std::cout << "NetworkTables are " << networkTableManager.IsConnected() ? "Connected" : "Not Connected";
			if(networkTableManager.IsConnected()){
				
			}
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	return 0;
}
