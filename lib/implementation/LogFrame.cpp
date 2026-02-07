#include <LogFrame.h>

#include <utility>
static timeval chrono_to_timeval(std::chrono::system_clock::time_point tp)
{
	auto us = time_point_cast<std::chrono::microseconds>(tp).time_since_epoch().count();
	timeval tv;
	tv.tv_sec = us / 1'000'000;
	tv.tv_usec = us % 1'000'000;
	return tv;
}

LogFrame::LogFrame(cv::Mat frame) :
	m_cvFrame(std::move(frame))
{
	m_pu8Img = m_cvFrame.data;
	m_nRows = m_cvFrame.rows;
	m_nCols = m_cvFrame.cols;
	m_nBuffer = m_cvFrame.channels();
	m_ts = chrono_to_timeval(std::chrono::system_clock::now());
}

cv::Mat& LogFrame::GetCVFrame() {
	return m_cvFrame;
}