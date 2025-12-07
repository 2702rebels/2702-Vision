#pragma once
#include <fstream>
#include <filesystem>
#include <functional>
#include <LogFrameType.h>
#include <opencv2/core.hpp>

namespace fs = std::filesystem;

class VisionLog {
	friend class VisionLogReader;
	friend class VisionLogWriter;
private:
	unsigned int m_resWidth;
	unsigned int m_resHeight;
	LogFrameType m_logType;
public:
	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	const LogFrameType& GetLogType() const;
};

class VisionLogReader : public VisionLog
{
private:
	std::ifstream m_logfile;
	std::unique_ptr<cv::Mat> ReadGrayscaleFrame();
	std::unique_ptr<cv::Mat> ReadRGBFrame();
	std::function<std::unique_ptr<cv::Mat>()> m_readFrameFun;
	int m_frameSize;
public:
	VisionLogReader(fs::path logPath);

	std::unique_ptr<cv::Mat> ReadFrame();
	void SkipToFrame(int frameNum);
};

class VisionLogWriter : public VisionLog {
private:
	std::ofstream m_logfile;
	int m_frameSize;
public:
	VisionLogWriter(fs::path logPath, LogFrameType frameType, unsigned int resWidth, unsigned int resHeight);
	
	void WriteFrame(const cv::Mat& frame);
};