#include <AsyncVisionLog.h>

AsyncVisionLog::AsyncVisionLog(fs::path logPath, unsigned int resWidth,
                               unsigned int resHeight,
                               LogFrameType logFrameType, int nWriterThreads)
    : m_syncWriter(logPath, logFrameType, resWidth, resHeight),
      m_boundWriterThread(std::bind(&AsyncVisionLog::WriterThread, this,
                                    std::placeholders::_1)),
      m_boundPusherThread(std::bind(&AsyncVisionLog::PusherThread, this,
                                    std::placeholders::_1)) {
  m_writingThreads.reserve(nWriterThreads);
  for (int i = 0; i < nWriterThreads; i++) {
    m_writingThreads.emplace(m_writingThreads.begin() + i, m_boundWriterThread);
  }
}

AsyncVisionLog::~AsyncVisionLog() {
  // Wait for any remaining frames to be added to the frame queue
  for (std::jthread &pusherThread : m_pusherThreads) {
    pusherThread.join();
  }
  while (true) {
    std::lock_guard<std::mutex> queueLock(m_frameQueueMtx);
    // Wait for the writer threads to finish writing all frames
    if (!m_frameQueue.empty()) {
      continue;
    }
    // Stop the writer threads
    for (std::jthread &writerThread : m_writingThreads) {
      writerThread.request_stop();
    }
    frameWaitingCondition.notify_all();
    break;
  }
  // Resource cleanup continues afterwards including joining of all alive
  // threads
}

// Idea: maybe batching will be better for performance than constantly sleeping
// and waking up the thread again? will bother with that after this works though

void AsyncVisionLog::WriterThread(std::stop_token cancellationToken) {
  while (!cancellationToken.stop_requested()) {
    std::unique_lock<std::mutex> lock(m_frameQueueMtx);
    frameWaitingCondition.wait(lock, [this, &cancellationToken] {
      return !this->m_frameQueue.empty() || cancellationToken.stop_requested();
    });
    if (cancellationToken.stop_requested()) {
      break;
    }
    m_syncWriter.WriteFrame(m_frameQueue.front());
    m_frameQueue.pop();
  }
}

void AsyncVisionLog::PusherThread(const cv::Mat &frame) {
  std::lock_guard<std::mutex> queueLock(m_frameQueueMtx);
  m_frameQueue.push(frame);
  frameWaitingCondition.notify_one();
}

void AsyncVisionLog::AddFrameToQueueSync(const cv::Mat &frame) {
  PusherThread(frame);
}

void AsyncVisionLog::AddFrameToQueueAsync(const cv::Mat &frame) {
  m_pusherThreads.emplace_back(m_boundPusherThread, frame);
}