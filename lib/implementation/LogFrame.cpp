#include <LogFrame.h>

LogFrame::LogFrame(std::unique_ptr<cv::Mat >> frame) :
	m_pu8Img(frame->data),
	m_nRows(frame->rows),
	m_nCols(frame->cols),
	m_nBuffer(frame->channels()),
	m_cvFrame(std::move(frame))
{

}

cv::Mat& LogFrame::GetCVFrame() {
	return *m_cvFrame;
}