/*! \brief Replays log files fetched from the pi through image processing.
 * This can be built and run on a desktop environment.
 * Usage is ./replay logfilepath
 * Depends on:
 * - opencv
 * - rpicam
 */

#include "LoggingUtils.h"

#include <string>

bool bStop = false;

#include "LogPlayer.h"

#include "ObjectFrameProcessor.h"

void OnInterrupt(int signal)
{
    bStop = true;
}

int main(int argc, char** argv) {
    signal(2, OnInterrupt);
    if (argc != 4) {
        error("Usage: {} inputfile width height", argv[0]);
        error("try 640x480 for width and height");
        return 1;
    }
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    ObjectFrameProcessor<ObjLatencyReporting::ON, ObjVideoDebuggingInfo::ON,
                         ObjUseGui::ON, ObjMultipleObjectDetection::OFF,
                         ObjSendPackets::OFF, ObjUseOutputFiltering::ON>
        ip(width, height);
    LogPlayer lp(argv[1], ip);
    while (!bStop) { //NOLINT
        if (!lp.bNextFrame()) {
            info("About to rewind");
            lp.vRewind();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
