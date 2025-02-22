/* Main for object detection. Cannot be run in a desktop testing environment. Uses NetworkTables to update object detection config at runtime.
Depends on: 
- opencv
- rpicam
- wpilib ntcore
- asio(UDP client library)
- object.h
- config.h
*/

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
#include <networktables/IntegerTopic.h>
#include <random>
#include <cmath>
#include <asio.hpp>
using asio::ip::udp;

using namespace std;

#include <signal.h>

bool bStop = false;

#include "object.h"
#include "config.h"

#define USE_NETWORK_TABLES 1
#define IS_ROBOT 0

/* Handles Ctrl+C interrupts
@param nSig - Should always be equal to @link SIGINT*/
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
		#if USE_NETWORK_TABLES
		auto networkTableManager = nt::NetworkTableInstance::GetDefault();
		std::mt19937 randomNumberGen = std::mt19937();
		std::uniform_int_distribution<uint32_t> networkTableRandomRange = std::uniform_int_distribution<uint32_t>(0,20);
		auto objectDetectionTable = networkTableManager.GetTable("ObjectDetection");
		nt::IntegerSubscriber hueMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/HueMin").Subscribe(0);
		nt::IntegerSubscriber hueMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/HueMax").Subscribe(0);
		nt::IntegerSubscriber saturationMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/SaturationMin").Subscribe(0);
		nt::IntegerSubscriber saturationMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/SaturationMax").Subscribe(0);
		nt::IntegerSubscriber valueMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/ValueMin").Subscribe(0);
		nt::IntegerSubscriber valueMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/ValueMax").Subscribe(0);
		networkTableManager.StartClient4("networkTableClient");
		networkTableManager.SetServerTeam(2702);
		#if IS_ROBOT
		networkTableManager.StartDSClient(); // Should only be run on actual robot
		#endif
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
			}
			#if USE_NETWORK_TABLES
			std::cout << "NetworkTables are " << (networkTableManager.IsConnected() ? "Connected" : "Not Connected") << "\n";
			if(networkTableManager.IsConnected() && networkTableRandomRange(randomNumberGen) == 1){
				std::cout << "Updated configs from network table\n";
				proc.setPVHueMin(hueMinTopic.GetAtomic().value);
				proc.setPVHueMax(hueMaxTopic.GetAtomic().value);
				proc.setSaturationMin(saturationMinTopic.GetAtomic().value);
				proc.setSaturationMax(saturationMaxTopic.GetAtomic().value);
				proc.setValueMin(valueMinTopic.GetAtomic().value);
				proc.setValueMax(valueMaxTopic.GetAtomic().value);
			}
			#endif
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	return 0;
}
