#include <LogFrame.h>
#include "LogPlayer.h"
#include "frmprocessor.h"

/*! \brief Creates a LogPlayer from a logfile path and a frame processor. 
* \param logfilePath - The logfile path to use.
* \param fp - The frame processor to use.
*/
LogPlayer::LogPlayer( fs::path logfilePath, FrmProcessor &fp ) :
	m_logReader(logfilePath),
	m_fp(fp)
{
}

/*! \brief Processes the next frame. 
* \returns true if rewind is needed, false otherwise. 
*/
bool LogPlayer::bNextFrame()
{
	std::unique_ptr<cv::Mat> nextFrame = m_logReader.ReadFrame();
	if (!nextFrame) {
		return false;
	}
	LogFrame lf(std::move(nextFrame));
	m_fp.ProcessFrame(0, &lf);
	return true;
}

/*! \brief Rewinds the video to the start. */
void LogPlayer::vRewind()
{
	m_logReader.SkipToFrame(0);
}

VisionLogReader& LogPlayer::GetLogReader() {
	return m_logReader;
}