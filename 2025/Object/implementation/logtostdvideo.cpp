/*! \brief Replays log files fetched from the pi through image processing.
 * This can be built and run on a desktop environment.
 * Usage is ./replay logfilepath
 * Depends on:
 * - opencv
 * - rpicam
 */

#include <LoggingUtils.h>

static constexpr int WIDTH = 640;
static constexpr int HEIGHT = 480;
static constexpr int FRAME_RATE = 30;
static constexpr int BITRATE = 40000;

#include "LogPlayer.h"
#include "logtostdvideo.h"

#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>

bool bStop = false;
namespace fs = std::filesystem;

StdVideoLogProc::StdVideoLogProc(const std::string& outputPath,
                                 const std::string& outputPathFiltered,
                                 const int width, const int height,
                                 const int fps, const int bitrate)
    : m_imageProcessor(width, height),
      m_logger(outputPath, width, height, fps, bitrate),
      m_filteredLogger(outputPathFiltered, width, height, fps, bitrate) {}

void StdVideoLogProc::ProcessFrame(int nCam, const CameraFrame* pcf) {
    cv::Mat frame(pcf->nRows(), pcf->nCols(), CV_8UC3, pcf->pu8Image());
    m_logger.LogFrame(frame);
    cv::Mat filteredFrame = m_imageProcessor.ProcessFrameWithRet(nCam, pcf);
    m_filteredLogger.LogFrame(filteredFrame);
    info("Finished writing frame {}", m_frameCount);
    m_frameCount++;
}

int main(int argc, char** argv) { // NOLINT
    SetupTextLogger();
    if (argc != 2) {
        error("Usage: {} inputdir|inputfile", argv[0]);
        return 1;
    }
    if (fs::is_directory(argv[1])) {
        info("Provided directory, using dirmode");
        for (const auto& entry : fs::directory_iterator(argv[1])) {
            if (entry.is_regular_file()) {
                StdVideoLogProc videoLogProc(
                    (entry.path().parent_path() /
                     (entry.path().filename().string() + "raw.mp4"))
                        .string(),
                    (entry.path().parent_path() /
                     (entry.path().filename().string() + "filtered.mp4"))
                        .string(),
                    WIDTH, HEIGHT, FRAME_RATE, BITRATE);
                LogPlayer lp(entry.path().c_str(), videoLogProc);
                while (true) {
                    if (bStop) {
                        break;
                    }
                    if (!lp.bNextFrame()) {
                        info("Succesfully translated file!");
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    } else if (fs::is_regular_file(argv[1])) {
        info("Provided file, using filemode");
        StdVideoLogProc videoLogProc(
            (fs::path(argv[1]).parent_path() /
             (fs::path(argv[1]).filename().string() + "raw.mp4"))
                .string(),
            (fs::path(argv[1]).parent_path() /
             ((fs::path(argv[1]).filename().string() + "filtered.mp4")))
                .string(),
            WIDTH, HEIGHT, FRAME_RATE, BITRATE);
        LogPlayer lp(argv[1], videoLogProc);
        while (true) {
            if (bStop) {
                return 0;
            }
            if (!lp.bNextFrame()) {
                info("Succesfully translated file!");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    } else {
        error("Usage: {} inputdir|inputfile", argv[0]);
        return 1;
    }
    return 0;
}
