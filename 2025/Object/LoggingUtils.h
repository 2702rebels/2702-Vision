#pragma once
#include <spdlog/spdlog.h>
#include "VisLog.h"
#include "object.h"
#include <string>
#include "LoggingUtils.h"

using namespace spdlog;

/** Default logger to print to console screen */
std::shared_ptr<logger> textLogsOutput;
void SetupTextLogger();
std::shared_ptr<logger> SetupFrameLogger(const char* name);

class FrameLogger : FrmProcessor {
private:
	std::shared_ptr<logger> m_unfilteredLogger;
	std::shared_ptr<logger> m_maskedLogger;
	std::shared_ptr<logger> m_annotatedLogger;
public:
	FrameLogger(const char* unfilteredName, const char* maskedName, const char* annotatedName);
	void LogUnfilteredFrame(const cv::Mat& frame, uint32_t u32Seq, uint32_t u32Exp);
	void LogMaskedFrame(const cv::Mat& frame, uint32_t u32Seq, uint32_t u32Exp);
	void LogAnnotatedFrame(const cv::Mat& frame, uint32_t u32Seq, uint32_t u32Exp);
};