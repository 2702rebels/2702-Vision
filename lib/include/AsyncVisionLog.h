#include <VisionLog.h>
#include <condition_variable>
#include <functional>
#include <memory>
#include <opencv2/imgproc.hpp>
#include <queue>
#include <thread>
#include <vector>

/**
 * @class AsyncVisionLog
 * @brief Asynchronously put frames to logfile on disk
 * @details Utilizes threads to not handle the job of syncing data to disk
 * synchronously. This object follows RAII by waiting for all logging to finish
 * before being destroyed
 */
class AsyncVisionLog {
private:
  /**
   * @brief the FILO queue of frames to log
   */
  std::queue<cv::Mat> m_frameQueue;
  /**
   * @brief Who has access to the @ref m_frameQueue
   */
  std::mutex m_frameQueueMtx;
  /**
   * @brief Struct to wake up and sleep the logging thread when there are frames
   * available in the @ref m_frameQueue
   */
  std::condition_variable frameWaitingCondition;

  /**
   * @brief The collection of threads running the @ref
   * AsyncVisionLog::WriterThread function to sync frames to disk
   */
  std::vector<std::jthread> m_writingThreads;
  /*
   * @brief The synchronous writer to do the actual logic of writing to disk
   */
  VisionLogWriter m_syncWriter;

  /**
   * @brief Removes elements from the @ref m_frameQueue and writes them to disk
   * using @ref AsyncVisionLog::m_syncWriter
   */
  void WriterThread(std::stop_token cancelToken);
  /**
   * @brief @ref AsyncVisionLog::WriterThread made into a lambda using std::bind
   */
  std::function<void(std::stop_token)> m_boundWriterThread;

  /**
   * @brief Collection of active @ref AsyncVisionLog::PusherThread
   */
  std::vector<std::jthread> m_pusherThreads;

  /**
   * @brief Function to take a @ref cv::Mat and push it into the queue
   */
  void PusherThread(const cv::Mat &frame);
  /**
   * @brief @ref AsyncVisionLog::PusherThread bound into a lambda using
   * std::bind
   */
  std::function<void(const cv::Mat &)> m_boundPusherThread;

public:
  /**
   * @brief Create a new logger from the @ref VisionLogWriter constructor
   * parameters and the number of writer threads to use
   */
  AsyncVisionLog(fs::path logPath, unsigned int resWidth,
                 unsigned int resHeight, LogFrameType logFrameType,
                 int nWriterThreads);
  /**
   * @brief Used to make sure threads exit in the right order: wait for pushers,
   * then writers, then free all other resources
   */
  ~AsyncVisionLog();

  /** @brief Synchronously adds the frame to the @ref m_frameQueue, but then the
   * writing to disk process is async */
  void AddFrameToQueueSync(const cv::Mat &frame);
  /** @brief Both the process of writing the frame to the @ref m_frameQueue and
   * the writing to disk process is async */
  void AddFrameToQueueAsync(const cv::Mat &frame);
};