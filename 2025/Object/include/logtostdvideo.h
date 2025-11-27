#pragma once
#include "VisLog.h"
#include <frmprocessor.h>
#include <string>
#include "LoggingUtils.h"
#include <spdlog/spdlog.h>
#include <ObjectFrameProcessor.h>

class StdVideoLogProc : public FrmProcessor {
    ObjectFrameProcessor imageProcessor;
    FILE* ffmpegRaw;
    FILE* ffmpegFiltered;
    public:
    StdVideoLogProc(std::string outputPath, std::string outputPathFiltered, int width, int height, int frame_rate): 
    imageProcessor("", width, height),
    ffmpegRaw(popen(((std::string)("ffmpeg -y "
        "-f rawvideo "
        "-pix_fmt rgb24 "
        "-s " + std::to_string(width) + "x" + std::to_string(height) + " "
        "-r " + std::to_string(frame_rate) + " "
        "-i - "
        "-c:v libx264 "
        "-preset ultrafast "
        "-tune zerolatency "
        "-qp 0 "
        "-f mp4 " + outputPath)).c_str(), "w")),
    ffmpegFiltered(popen(((std::string)("ffmpeg -y "
        "-f rawvideo "
        "-pix_fmt rgb24 "
        "-s " + std::to_string(width) + "x" + std::to_string(height) + " "
        "-r " + std::to_string(frame_rate) + " "
        "-i - "
        "-c:v libx264 "
        "-preset ultrafast "
        "-tune zerolatency "
        "-qp 0 "
        "-f mp4 " + outputPathFiltered)).c_str(), "w"))
    {
    };

    virtual void ProcessFrame(int nCam, CameraFrame const *pcf){
        cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image() );
        fwrite(static_cast<const void*>(frame.data), 1, frame.total() * frame.elemSize(), ffmpegRaw);
        cv::Mat filteredFrame = imageProcessor.ProcessFrameWithRet(nCam, pcf);
        fwrite(static_cast<const void*>(filteredFrame.data), 1, filteredFrame.total() * filteredFrame.elemSize(), ffmpegFiltered);
        info("Finished writing frame {}", pcf->u32Sequence());
    }

    virtual void LogEnd(){
        pclose(ffmpegRaw);
        pclose(ffmpegFiltered);
    }
};