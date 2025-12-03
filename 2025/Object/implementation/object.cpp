/*! \brief Main for object detection. Cannot be run in a desktop testing environment. Uses NetworkTables to update object detection config at runtime.
 * Depends on:
 *  - opencv
 *  - rpicam
 *  - wpilib ntcore
 *  - asio(UDP client library)
 *  - object.h
 *  - config.h
 */
#include <errno.h>
#include "LoggingUtils.h"

 // #include <X11/Xlib.h>
 // #include <cameraserver/CameraServer.h>
 // #include <networktables/NetworkTableInstance.h>
#include <drm/drm_fourcc.h>

#include <linux/videodev2.h>

#include "rpicam.h"

#include <signal.h>

bool bStop = false;

#include "ObjectFrameProcessor.h"

#define USE_NETWORK_TABLES 1
#define IS_ROBOT 1
/*! \brief Fixed exposure time in us that is fed to rpicam. */
#define FIXED_EXPOSURE_TIME 75000
// waterloo comp #define FIXED_GAIN_VALUE 1.4
#define FIXED_GAIN_VALUE 5 // practice field 2025

#define RESOLUTION_WIDTH 640
#define RESOLUTION_HEIGHT 480

/*! \brief Directory where to log files to. */
#define LOG_DIR "/data/logs/"

/*! \brief Handles Ctrl+C interrupts.
 * \param nSig - Should always be equal to \ref SIGINT.*/
void sighandler(int nSig)
{
	info("Received Signal {}", nSig);
	bStop = true;
}

int main(int argc, char* argv[])
{
	SetupTextLogger();
	signal(SIGINT, sighandler);
	try
	{
		ObjectFrameProcessor proc(RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
		//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		RPICamera mycam0("/dev/video0", 0, RESOLUTION_WIDTH, RESOLUTION_HEIGHT, DRM_FORMAT_RGB888, proc, FIXED_EXPOSURE_TIME, FIXED_GAIN_VALUE);
		//	nvCamera mycam1( "/dev/video1", 4, 1 );

		//	printinfo( mycam0 );
#if USE_NETWORK_TABLES
		auto networkTableManager = nt::NetworkTableInstance::GetDefault();
		auto visionTable = networkTableManager.GetTable("Vision");
		nt::BooleanSubscriber isLoggingTopic = visionTable->GetBooleanTopic("IsLogging").Subscribe(false);
		bool bLogStarted = false;
		networkTableManager.StartClient4("networkTableClient");
		networkTableManager.SetServerTeam(2702);
#if IS_ROBOT
		networkTableManager.StartDSClient();
#endif
		while (!bStop)
		{
			info("Connection status of NetworkTables: {}", networkTableManager.IsConnected());
			if (networkTableManager.IsConnected())
			{
				info("net tables connected");
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
	catch (std::exception& e)
	{
		error("Exception: {}", e.what());
	}

	return 0;
}

