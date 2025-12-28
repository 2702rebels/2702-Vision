#include <VisionLog.h>
#include <spdlog/spdlog.h>
#include <cassert>

constexpr int LOGFILE_HEADER_SIZE = 12;

unsigned int VisionLog::GetWidth() const {
	return m_resWidth;
}

unsigned int VisionLog::GetHeight() const {
	return m_resHeight;
}

const LogFrameType& VisionLog::GetLogType() const {
	return m_logType;
}

VisionLogReader::VisionLogReader(fs::path logPath) :
	m_logfile(logPath, std::ios::in | std::ios::binary)
{
	if (!m_logfile.read((char*)&m_logType, sizeof(m_logType))) {
		spdlog::error("Failed to read log type");
		throw;
	}
	if (!m_logfile.read((char*)&m_resWidth, sizeof(m_resWidth))) {
		spdlog::error("Failed to read resolution width");
		throw;
	}
	if (!m_logfile.read((char*)&m_resHeight, sizeof(m_resHeight))) {
		spdlog::error("Failed to read resolution height");
		throw;
	}
	if (m_logType == LogFrameType::GRAYSCALE) {
		m_readFrameFun = std::bind(&VisionLogReader::TryReadGrayscaleFrame, this);
		m_frameSize = m_resHeight * m_resWidth;
	}
	else if (m_logType == LogFrameType::RGB) {
		m_readFrameFun = std::bind(&VisionLogReader::TryReadRGBFrame, this);
		m_frameSize = m_resHeight * m_resWidth * 3;
	}
	else {
		spdlog::error("Invalid frame type: {}", (unsigned int)m_logType);
		throw;
	}
	if (m_resWidth > 4000) {
		spdlog::error("Resolution width looking funny (>4000)");
		throw;
	}
	if (m_resHeight > 4000) {
		spdlog::error("Resolution height looking funny (>4000)");
		throw;
	}
}

std::optional<cv::Mat> VisionLogReader::TryReadGrayscaleFrame() {
	cv::Mat frame(m_resHeight, m_resWidth, CV_8UC1);
	if (!m_logfile.read((char*)frame.data, m_frameSize)) {
		return std::nullopt;
	}
	return frame;
}

std::optional<cv::Mat> VisionLogReader::TryReadRGBFrame() {
	cv::Mat frame(m_resHeight, m_resWidth, CV_8UC3);
	if (!m_logfile.read((char*)frame.data, m_frameSize)) {
		return std::nullopt;
	}
	return frame;
}

cv::Mat VisionLogReader::ReadFrame() {
	std::optional<cv::Mat> frame = m_readFrameFun();
	if (!frame.has_value()) {
		spdlog::error("Hard readFrame was called and failed to read the frame");
		throw std::runtime_error("Hard readFrame was called and failed to read the frame");
	}
	return frame.value();
}

std::optional<cv::Mat> VisionLogReader::TryReadFrame() {
	return m_readFrameFun();
}

void VisionLogReader::SkipToFrame(int frameNum) {
	m_logfile.clear(); // clear the end-of-file(EOF) bit manually as seekg does not reset it and reads will fail if it is still set
	m_logfile.seekg(LOGFILE_HEADER_SIZE + m_frameSize * frameNum, std::ios::beg);
	if (!m_logfile)
	{
		spdlog::error("Failed to rewind ifstream ptr to start of file");
	}
}

VisionLogWriter::VisionLogWriter(fs::path logPath, LogFrameType frameType, unsigned int resWidth, unsigned int resHeight) :
	m_logfile(logPath, std::ios::out | std::ios::binary | std::ios::trunc)
{
	m_resWidth = resWidth;
	m_resHeight = resHeight;
	m_logType = frameType;
	m_logfile.write((char*)&m_logType, sizeof(m_logType));
	m_logfile.write((char*)&m_resWidth, sizeof(m_resWidth));
	m_logfile.write((char*)&m_resHeight, sizeof(m_resHeight));
	if (frameType == LogFrameType::GRAYSCALE) {
		m_frameSize = m_resWidth * m_resHeight;
	}
	else if (frameType == LogFrameType::RGB) {
		m_frameSize = m_resWidth * m_resHeight * 3;
	}
	else {
		spdlog::error("Unrecognized frame type: {}", (unsigned int)frameType);
		throw;
	}
}

void VisionLogWriter::WriteFrame(const cv::Mat& frame) {
	m_logfile.write((char*)frame.data, m_frameSize);
}