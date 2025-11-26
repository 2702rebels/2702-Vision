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

#include "object.h"
#include "config.h"

int main( int argc, char **argv )
{
	if (argc != 2) {
		error("Usage: {} inputfile", argv[0] );
		return 1;
	}
	
	imgProc ip;
	LogPlayer lp( argv[1], ip );
	ConfigUtils::triggerConfigLoad(ip);
	while (1) {
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
