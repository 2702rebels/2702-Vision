#include <VisionLog.h>
#include <spdlog/spdlog.h>
#include <cassert>

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
		m_readFrameFun = std::bind(&VisionLogReader::ReadGrayscaleFrame, this);
		m_frameSize = m_resHeight * m_resWidth;
	}
	else if (m_logType == LogFrameType::RGB) {
		m_readFrameFun = std::bind(&VisionLogReader::ReadRGBFrame, this);
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

std::unique_ptr<cv::Mat> VisionLogReader::ReadGrayscaleFrame() {
	std::unique_ptr<cv::Mat> frame = std::make_unique<cv::Mat>(m_resHeight, m_resWidth, CV_8UC1);
	if (!m_logfile.read((char*)frame->data, m_frameSize)) {
		return nullptr;
	}
	return std::move(frame);
}

std::unique_ptr<cv::Mat> VisionLogReader::ReadRGBFrame() {
	std::unique_ptr<cv::Mat> frame = std::make_unique<cv::Mat>(m_resHeight, m_resWidth, CV_8UC3);
	if (!m_logfile.read((char*)frame->data, m_frameSize)) {
		return nullptr;
	}
	return std::move(frame);
}

std::unique_ptr<cv::Mat> VisionLogReader::ReadFrame() {
	return m_readFrameFun();
}

void VisionLogReader::SkipToFrame(int frameNum) {
	if (m_logType == LogFrameType::GRAYSCALE) {
		m_logfile.seekg(12 + m_resWidth * m_resHeight * frameNum, std::ios::beg);
	}
	else if (m_logType == LogFrameType::RGB) {
		m_logfile.seekg(12 + m_resWidth * m_resHeight * 3 * frameNum, std::ios::beg);
	}
	else {
		spdlog::error("Unrecognized frame type");
		throw;
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