#include <FFMPEGLogger.h>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

using namespace spdlog;

FFMPEGLogger::FFMPEGLogger(const std::string &filepath, int width, int height,
                           int fps, int bitrate) {
  avformat_network_init();

  // Allocate the output media context
  avformat_alloc_output_context2(&m_logCtx, nullptr, nullptr, filepath.c_str());
  if (!m_logCtx) {
    error("Could not allocate format context");
    throw;
  }

  // Find encoder
  m_logCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!m_logCodec) {
    error("Codec not found");
    throw;
  }

  // Create new stream
  m_logStream = avformat_new_stream(m_logCtx, m_logCodec);
  if (!m_logStream) {
    error("Could not create stream");
    throw;
  }

  // Set codec parameters
  m_logCodecCtx = avcodec_alloc_context3(m_logCodec);
  m_logCodecCtx->codec_id = m_logCodec->id;
  m_logCodecCtx->bit_rate = bitrate;
  m_logCodecCtx->width = width;
  m_logCodecCtx->height = height;
  m_logCodecCtx->time_base = AVRational{1, fps};
  m_logCodecCtx->framerate = AVRational{fps, 1};
  m_logCodecCtx->gop_size = 10;
  m_logCodecCtx->max_b_frames = 1;
  m_logCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

  if (m_logCtx->oformat->flags & AVFMT_GLOBALHEADER)
    m_logCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  avcodec_open2(m_logCodecCtx, m_logCodec, nullptr);
  avcodec_parameters_from_context(m_logStream->codecpar, m_logCodecCtx);

  // Open output file
  if (!(m_logCtx->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&m_logCtx->pb, filepath.c_str(), AVIO_FLAG_WRITE) < 0) {
      error("Could not open output file");
      throw;
    }
  }

  // Write header
  avformat_write_header(m_logCtx, nullptr);
  swsCtx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height,
                          AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr,
                          nullptr);
}

void FFMPEGLogger::LogFrame(const cv::Mat &originalFrame) {
  cv::Mat frame;
  cv::cvtColor(originalFrame, frame, cv::COLOR_BGR2RGB);
  AVFrame *curFrame = av_frame_alloc();
  curFrame->format = m_logCodecCtx->pix_fmt;
  curFrame->width = frame.cols;
  curFrame->height = frame.rows;
  av_frame_get_buffer(curFrame, 32);
  uint8_t *src_slices[1] = {frame.data};
  int srcStride[1] = {static_cast<int>(frame.step)};
  sws_scale(swsCtx, src_slices, srcStride, 0, frame.rows, curFrame->data,
            curFrame->linesize);
  curFrame->pts = m_frameIndex++;
  AVPacket packet;
  av_init_packet(&packet);
  avcodec_send_frame(m_logCodecCtx, curFrame);
  while (avcodec_receive_packet(m_logCodecCtx, &packet) == 0) {
    av_interleaved_write_frame(m_logCtx, &packet);
    av_packet_unref(&packet);
  }
}

FFMPEGLogger::~FFMPEGLogger() { av_write_trailer(m_logCtx); }
