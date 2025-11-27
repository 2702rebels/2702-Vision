/*! \brief Replays log files fetched from the pi through image processing. 
* This can be built and run on a desktop environment.
* Usage is ./replay logfilepath
* Depends on: 
* - opencv
* - rpicam
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
#include "LoggingUtils.h"

using namespace std;

#include <signal.h>

bool bStop = false;

#include "LogPlayer.h"

#define IMGPROC_GUI	1

#include "ObjectFrameProcessor.h"
#include "config.h"

int main( int argc, char **argv )
{
	if (argc != 4) {
		error("Usage: {} inputfile width height", argv[0] );
		error("try 640x480 for width and height");
		return 1;
	}
	int width = stoi(argv[2]);
	int height = stoi(argv[3]);
	ObjectFrameProcessor ip("/", width, height);
	LogPlayer lp( argv[1], ip );
	ConfigUtils::triggerConfigLoad(ip);
	while (true) {
		if(ConfigUtils::configHasChanged(ip)){
			ConfigUtils::triggerConfigSave(ip);
		}
		if (!lp.bNextFrame()) {
			info( "About to rewind" );
			lp.vRewind();
		}
		sleep( 1 );
	}


	return 0;
}
