#include <LogFrame.h>

#include <utility>

LogFrame::LogFrame(cv::Mat frame) :
	m_cvFrame(std::move(frame))
{
	m_pu8Img = m_cvFrame.data;
	m_nRows = m_cvFrame.rows;
	m_nCols = m_cvFrame.cols;
	m_nBuffer = m_cvFrame.channels();
	m_ts = std::chrono::system_clock::now();
}

cv::Mat& LogFrame::GetCVFrame() {
	return m_cvFrame;
}