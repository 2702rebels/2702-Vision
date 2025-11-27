/*! \brief Replays log files fetched from the pi through image processing. 
* This can be built and run on a desktop environment.
* Usage is ./replay logfilepath
* Depends on: 
* - opencv
* - rpicam
*/

#include <string>

#include "rpicam.h"
#include "LoggingUtils.h"

#include <signal.h>

bool bStop = false;

#include "LogPlayer.h"

#define IMGPROC_GUI	1

#include "ObjectFrameProcessor.h"

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
