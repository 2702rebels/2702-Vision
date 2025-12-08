#include <VisionLog.h>
#include <thread>
#include <queue>
#include <condition_variable>
#include <vector>
#include <memory>
#include <opencv2/imgproc.hpp>
#include <functional>

class AsyncVisionLog {
private:
	std::queue<std::shared_ptr<cv::Mat>> m_frameQueue;
	std::mutex m_frameQueueMtx;
	std::condition_variable frameWaitingCondition;

	std::vector<std::jthread> m_writingThreads;
	VisionLogWriter m_syncWriter;

	void WriterThread(std::stop_token cancelToken);
	std::function<void(std::stop_token)> m_boundWriterThread;

	std::vector<std::jthread> m_pusherThreads;

	void PusherThread(std::shared_ptr<cv::Mat> frame);
	std::function<void(std::shared_ptr<cv::Mat>)> m_boundPusherThread;
public:
	AsyncVisionLog(fs::path logPath, unsigned int resWidth, unsigned int resHeight, LogFrameType logFrameType, int nWriterThreads);
	~AsyncVisionLog();

	/** Synchronously adds the frame to the queue, but then the writing to disk process is async */
	void AddFrameToQueueSync(std::shared_ptr<cv::Mat> frame);
	/** Both the process of writing the frame to the queue and the writing to disk process is async */
	void AddFrameToQueueAsync(std::shared_ptr<cv::Mat> frame);
};