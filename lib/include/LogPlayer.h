#pragma once
#include "VisionLog.h"
#include "frmprocessor.h"

/**
 * @class LogPlayer
 * @brief A class for replaying a VisionLog into a @ref FrmProcessor
 */
class LogPlayer : public VisionLog {
  /** @brief The frame processor to feed the read frames into */
  FrmProcessor &m_fp;
  /** @brief The reader of the logfile */
  VisionLogReader m_logReader;

public:
  /**
   * @brief Create a new @ref LogPlayer from the path to the logfile and the
   * processor to feed the frames through
   * @param logfilePath passed to the @ref VisionLogReader constructor to read
   * the log
   * @param fp The frame processor which will receive the frames read
   */
  LogPlayer(fs::path logfilePath, FrmProcessor &fp);

  /** @brief Feeds the next frame through the @ref m_fp
   * @returns true if the frame was successfully fed and false if it was not
   */
  bool bNextFrame();
  /** @brief Getter for @ref m_logReader */
  VisionLogReader &GetLogReader();
  /** @brief Rewind the @ref m_logReader to frame 0 */
  void vRewind();
};
