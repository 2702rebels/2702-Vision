#include <LogFrame.h>

LogFrame::LogFrame(const cv::Mat& frame) :
	CameraFrame(),
	m_cvFrame(std::move(frame))
{
	m_pu8Img = m_cvFrame.data;
	m_nRows = m_cvFrame.rows;
	m_nCols = m_cvFrame.cols;
	m_nBuffer = m_cvFrame.channels();
}

cv::Mat& LogFrame::GetCVFrame() {
	return m_cvFrame;
}