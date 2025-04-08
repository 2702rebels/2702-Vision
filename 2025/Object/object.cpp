/*! \brief Main for object detection. Cannot be run in a desktop testing environment. Uses NetworkTables to update object detection config at runtime.
 * Depends on:
 *  - opencv
 *  - rpicam
 *  - wpilib ntcore
 *  - asio(UDP client library)
 *  - object.h
 *  - config.h
 */

#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

// #include <X11/Xlib.h>
// #include <cameraserver/CameraServer.h>
// #include <networktables/NetworkTableInstance.h>
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
#include <networktables/DoubleTopic.h>
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
#define IS_ROBOT 1
/*! \brief Fixed exposure time in us that is fed to rpicam. */
#define FIXED_EXPOSURE_TIME 75000
// waterloo comp #define FIXED_GAIN_VALUE 1.4
#define FIXED_GAIN_VALUE 5 // practice field 2025

#define RESOLUTION_WIDTH 640
#define RESOLUTION_HEIGHT 480

/*! \brief Handles Ctrl+C interrupts.
 * \param nSig - Should always be equal to \ref SIGINT.*/
void sighandler(int nSig)
{
	printf("Received Signal %d\n", nSig);
	bStop = true;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighandler);
#if 0
	char szImgName[100];
	sprintf( szImgName, "Testing" );
	cv::namedWindow( szImgName, cv::WINDOW_AUTOSIZE );
	cv::Mat img = cv::imread("img.png", cv::IMREAD_COLOR);
	printf( "img size: %d x %d\n", img.rows, img.cols );
	cv::imshow( szImgName, img );
	cv::waitKey(0);
#endif
	try
	{
		imgProc proc;
		//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		RPICamera mycam0("/dev/video0", 0, RESOLUTION_WIDTH, RESOLUTION_HEIGHT, DRM_FORMAT_RGB888, proc, FIXED_EXPOSURE_TIME, FIXED_GAIN_VALUE);
		//	nvCamera mycam1( "/dev/video1", 4, 1 );

		//	printinfo( mycam0 );
		ConfigUtils::triggerConfigLoad(proc);
		while (!bStop)
		{
			if (ConfigUtils::configHasChanged(proc))
			{
				ConfigUtils::triggerConfigSave(proc);
			}
			if (cv::waitKey(1) == 'l')
			{
				ConfigUtils::triggerConfigLoad(proc);
			}
#if USE_NETWORK_TABLES
			auto networkTableManager = nt::NetworkTableInstance::GetDefault();
			auto objectDetectionTable = networkTableManager.GetTable("ObjectDetection");
			auto visionTable = networkTableManager.GetTable("Vision");
			nt::IntegerSubscriber hueMinTopic = objectDetectionTable->GetIntegerTopic("HueMin").Subscribe(proc.getPVHueMin());
			nt::IntegerSubscriber hueMaxTopic = objectDetectionTable->GetIntegerTopic("HueMax").Subscribe(proc.getPVHueMax());
			nt::IntegerSubscriber saturationMinTopic = objectDetectionTable->GetIntegerTopic("SaturationMin").Subscribe(proc.getSaturationMin());
			nt::IntegerSubscriber saturationMaxTopic = objectDetectionTable->GetIntegerTopic("SaturationMax").Subscribe(proc.getSaturationMax());
			nt::IntegerSubscriber valueMinTopic = objectDetectionTable->GetIntegerTopic("ValueMin").Subscribe(proc.getValueMin());
			nt::IntegerSubscriber valueMaxTopic = objectDetectionTable->GetIntegerTopic("ValueMax").Subscribe(proc.getValueMax());
			nt::IntegerSubscriber minPixelWidthTopic = objectDetectionTable->GetIntegerTopic("MinPixelWidth").Subscribe(proc.getMinPixelWidth());
			nt::IntegerSubscriber minPixelHeightTopic = objectDetectionTable->GetIntegerTopic("MinPixelHeight").Subscribe(proc.getMinPixelHeight());
			nt::IntegerSubscriber minPixelAreaTopic = objectDetectionTable->GetIntegerTopic("MinPixelArea").Subscribe(proc.getMinPixelArea());
			nt::IntegerSubscriber maxPixelWidthTopic = objectDetectionTable->GetIntegerTopic("MaxPixelWidth").Subscribe(proc.getMaxPixelWidth());
			nt::IntegerSubscriber maxPixelHeightTopic = objectDetectionTable->GetIntegerTopic("MaxPixelHeight").Subscribe(proc.getMaxPixelHeight());
			nt::IntegerSubscriber maxPixelAreaTopic = objectDetectionTable->GetIntegerTopic("MaxPixelArea").Subscribe(proc.getMaxPixelArea());
			nt::DoubleSubscriber  minPercentFillTopic = objectDetectionTable->GetDoubleTopic("MinPercentFill").Subscribe(proc.getMinPercentFill());
			nt::BooleanSubscriber isLoggingTopic = visionTable->GetBooleanTopic("IsLogging").Subscribe(false);
			bool bLogStarted = false;
			networkTableManager.StartClient4("networkTableClient");
			networkTableManager.SetServerTeam(2702);
#if IS_ROBOT
			networkTableManager.StartDSClient();
#endif
			std::cout << "NetworkTables are " << (networkTableManager.IsConnected() ? "Connected" : "Not Connected") << "\n";
			if (networkTableManager.IsConnected())
			{
				std::cout << "Updated configs from network table\n";
				proc.setPVHueMin(hueMinTopic.GetAtomic().value);
				proc.setPVHueMax(hueMaxTopic.GetAtomic().value);
				proc.setSaturationMin(saturationMinTopic.GetAtomic().value);
				proc.setSaturationMax(saturationMaxTopic.GetAtomic().value);
				proc.setValueMin(valueMinTopic.GetAtomic().value);
				proc.setValueMax(valueMaxTopic.GetAtomic().value);
				if (!bLogStarted)
				{
					if (isLoggingTopic.Get())
					{
						proc.vStartLogging();
						bLogStarted = true;
					}
				}
			}
#endif
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}
	catch (std::exception &e)
	{
		printf("Exception: %s\n", e.what());
	}

	return 0;
}

