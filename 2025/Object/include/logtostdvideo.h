#pragma once
#include <FFMPEGLogger.h>
#include <ObjectFrameProcessor.h>
#include <frmprocessor.h>
#include <string>

class StdVideoLogProc : public FrmProcessor {
    ObjectFrameProcessor<ObjLatencyReporting::ON, ObjVideoDebuggingInfo::ON,
                         ObjUseGui::OFF, ObjMultipleObjectDetection::OFF,
                         ObjSendPackets::OFF, ObjUseOutputFiltering::ON>
        m_imageProcessor;
    FFMPEGLogger m_logger;
    FFMPEGLogger m_filteredLogger;
    int m_frameCount = 0;

  public:
    StdVideoLogProc(const std::string& outputPath,
                    const std::string& outputPathFiltered, int width, int height,
                    int fps, int bitrate);

    void ProcessFrame(int nCam, const CameraFrame* pcf) override;
};
