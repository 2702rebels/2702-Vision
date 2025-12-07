#include <frmprocessor.h>
#include <memory>
#include <opencv2/imgproc.hpp>

class LogFrame : public CameraFrame {
private:
	std::unique_ptr<cv::Mat> m_cvFrame;
public:
	LogFrame(std::unique_ptr<cv::Mat> frame);

	cv::Mat& GetCVFrame();
};