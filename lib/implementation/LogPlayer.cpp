#include "LogPlayer.h"
#include "frmprocessor.h"
#include <LogFrame.h>

/*! \brief Creates a LogPlayer from a logfile path and a frame processor.
 * \param logfilePath - The logfile path to use.
 * \param fp - The frame processor to use.
 */
LogPlayer::LogPlayer(fs::path logfilePath, FrmProcessor &fp)
    : m_logReader(logfilePath), m_fp(fp) {}

/*! \brief Processes the next frame.
 * \returns true if rewind is needed, false otherwise.
 */
bool LogPlayer::bNextFrame() {
  std::optional<cv::Mat> nextFrame = m_logReader.TryReadFrame();
  if (!nextFrame) {
    return false;
  }
  LogFrame lf(nextFrame.value());
  m_fp.ProcessFrame(0, &lf);
  return true;
}

/*! \brief Rewinds the video to the start. */
void LogPlayer::vRewind() { m_logReader.SkipToFrame(0); }

VisionLogReader &LogPlayer::GetLogReader() { return m_logReader; }