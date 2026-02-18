#pragma once
#include <opencv2/core/mat.hpp>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

/**
 * @class FFMPEGLogger
 * @brief Logs BGR @ref cv::Mat frames to mp4 files through ffmpeg
 * @warning Provided frames must be @ref CV_8UC3 type BGR frames
 */
class FFMPEGLogger {
private:
  AVFormatContext *m_logCtx;
  const AVCodec *m_logCodec;
  AVStream *m_logStream;
  AVCodecContext *m_logCodecCtx;
  struct SwsContext *swsCtx;
  int m_frameIndex = 0;

public:
  /** @brief Create a new output file from a filepath, frame resolution, and
   * output video framerate and bitrate
   * @param filepath The path of the output file to write to
   * @param width The width of the inputted frames
   * @param height The height of the inputted frames
   * @param fps The framerate of the outputted video
   * @param bitrate The bitrate of the outputted video
   */
  FFMPEGLogger(const std::string &filepath, int width, int height, int fps,
               int bitrate);
  ~FFMPEGLogger();
  /** @brief Log another frame into the video
   * @param frame A BGR openCV frame
   * @warning Frame is assumed to be in BGR format and have resolution specified
   * in the constructor
   */
  void LogFrame(const cv::Mat &frame);
};
