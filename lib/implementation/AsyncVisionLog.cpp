#include <AsyncVisionLog.h>

AsyncVisionLog::AsyncVisionLog(fs::path logPath, unsigned int resWidth, unsigned int resHeight, LogFrameType logFrameType, int nWriterThreads) :
	m_syncWriter(logPath, logFrameType, resWidth, resHeight),
	m_boundWriterThread(std::bind(&AsyncVisionLog::WriterThread, this, std::placeholders::_1)),
	m_boundPusherThread(std::bind(&AsyncVisionLog::PusherThread, this, std::placeholders::_1))
{
	m_writingThreads.reserve(nWriterThreads);
	for (int i = 0; i < nWriterThreads; i++) {
		m_writingThreads.emplace(
			m_writingThreads.begin() + i,
			m_boundWriterThread
		);
	}
}

// Idea: maybe batching will be better for performance than constantly sleeping and waking up the thread again? will bother with that after this works though

void AsyncVisionLog::WriterThread(std::stop_token cancellationToken) {
	while (!cancellationToken.stop_requested()) {
		std::unique_lock<std::mutex> lock(m_frameQueueMtx);
		frameWaitingCondition.wait(lock, [this] {
			return !this->m_frameQueue.empty();
		});
		m_syncWriter.WriteFrame(*m_frameQueue.front());
		m_frameQueue.pop();
	}
}

void AsyncVisionLog::PusherThread(std::unique_ptr<cv::Mat> frame) {
	std::lock_guard<std::mutex> queueLock(m_frameQueueMtx);
	m_frameQueue.push(std::move(frame));
	frameWaitingCondition.notify_one();
}

void AsyncVisionLog::AddFrameToQueueSync(std::unique_ptr<cv::Mat> frame) {
	PusherThread(std::move(frame));
}

void AsyncVisionLog::AddFrameToQueueAsync(std::unique_ptr<cv::Mat> frame) {
	m_pusherThreads.emplace_back(m_boundPusherThread, std::move(frame));
}