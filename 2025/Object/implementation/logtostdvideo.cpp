/*! \brief Replays log files fetched from the pi through image processing.
 * This can be built and run on a desktop environment.
 * Usage is ./replay logfilepath
 * Depends on:
 * - opencv
 * - rpicam
 */

#include <LoggingUtils.h>

using namespace std;

#include <signal.h>

#define WIDTH 640
#define HEIGHT 480
#define FRAME_RATE 20

#include "LogPlayer.h"
#include "logtostdvideo.h"

#include <string>
#include <signal.h>
#include <iostream>
#include <filesystem>

bool bStop = false;
namespace fs = std::filesystem;

/*! \brief This is called when Ctrl+C is pressed
 * \param nSig - The signal code that was interrupted, should always be 2 for Ctrl+C
 */
void sighandler(int nSig)
{
	info("Received Signal: {}", nSig);
	bStop = true;
}

int main(int argc, char **argv)
{
	SetupTextLogger();
	if (argc != 2)
	{
		error("Usage: {} inputdir|inputfile", argv[0]);
		return 1;
	}
	if(fs::is_directory(argv[1])){
		info("Provided directory, using dirmode");
		for (const auto &entry : fs::directory_iterator(argv[1]))
		{
			if (entry.is_regular_file())
			{
				StdVideoLogProc videoLogProc = StdVideoLogProc((entry.path().parent_path() / (entry.path().filename().string() + "raw.mp4")).string(), (entry.path().parent_path() / (entry.path().filename().string() + "filtered.mp4")).string(), WIDTH, HEIGHT, FRAME_RATE);
				LogPlayer lp(entry.path().c_str(), videoLogProc);
				while (true)
				{
					if (bStop)
					{
						videoLogProc.LogEnd();
						return 0;
					}
					if (!lp.bNextFrame())
					{
						info("Succesfully translated file!");
						videoLogProc.LogEnd();
						break;
					}
					sleep(1);
				}
			}
		}
	}
	else if(fs::is_regular_file(argv[1])){
		info("Provided file, using filemode");
		StdVideoLogProc videoLogProc = StdVideoLogProc((fs::path(argv[1]).parent_path() / (fs::path(argv[1]).filename().string() + "raw.mp4")).string(), (fs::path(argv[1]).parent_path() / ((fs::path(argv[1]).filename().string() + "filtered.mp4"))).string(), WIDTH, HEIGHT, FRAME_RATE);
		LogPlayer lp(argv[1], videoLogProc);
		while (true)
		{
			if (bStop)
			{
				videoLogProc.LogEnd();
				return 0;
			}
			if (!lp.bNextFrame())
			{
				info("Succesfully translated file!");
				videoLogProc.LogEnd();
				break;
			}
			sleep(1);
		}
	}
	else{
		error("Usage: {} inputdir|inputfile", argv[0]);
		return 1;
	}
	return 0;
}
