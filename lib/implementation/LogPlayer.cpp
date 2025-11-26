#include <stdint.h>
#include <sys/time.h>

#include "LogPlayer.h"

class LogFrame : public CameraFrame
{
	/*! \brief OpenCV material containing images to process. */
	cv::Mat		&m_mat;
	/*! \brief Exposure value to use. */
	uint32_t	m_u32Exp;
	/*! \brief Current frame number. */
	uint32_t	m_u32Seq;
public:
	LogFrame( cv::Mat &m, uint32_t u32Seq, uint32_t u32Exp ) :
		CameraFrame(),
		m_mat(m),
		m_u32Exp( u32Exp ),
		m_u32Seq( u32Seq )
	{
		m_nRows = m_mat.rows;
		m_nCols = m_mat.cols;
		gettimeofday( &m_ts, 0 );
	}
	virtual ~LogFrame()
	{
	}
	/*! \brief Returns the current frame number. */
	virtual uint32_t u32Sequence() const
	{
		return m_u32Seq;
	}
	/*! \brief Returns the exposure being used to process the frames. */
	virtual uint32_t u32Exposure() const
	{
		return m_u32Exp;
	}
	/*! \brief Returns the image height */
	virtual int nRows() const
	{
		return m_mat.rows;
	}
	/*! \brief Returns the image width */
	virtual int nCols() const
	{
		return m_mat.cols;
	}
	/*! \brief Returns a pointer to the openCV material */
	virtual uint8_t const *pu8Image() const
	{
		return m_mat.ptr();
	}
	/*! \brief Returns a pointer to the openCV material */
	virtual uint8_t *pu8Image()
	{
		return m_mat.ptr();
	}
	virtual const struct timeval *ts() const
	{
		return &m_ts;
	}
};

/*! \brief Creates a LogPlayer from a logfile path and a frame processor. 
* \param psz - The logfile path to use.
* \param fp - The frame processor to use.
*/
LogPlayer::LogPlayer( char const *psz, FrmProcessor &fp ) :
	VisionLog( psz, false ),
	m_fp( fp )
{
}

LogPlayer::~LogPlayer()
{
}

/*! \brief Processes the next frame. 
* \returns true if rewind is needed, false otherwise. 
*/
bool LogPlayer::bNextFrame()
{
	cv::Mat mat;
	int nExp;
	int nSeq = i32ReadNext( mat, nExp );
	if (nSeq < 0) {
		return false;
	}

	LogFrame lf( mat, nSeq, nExp );
	m_fp.ProcessFrame( 0, &lf );
	
	return true;
}

/*! \brief Rewinds the video to the start. */
void LogPlayer::vRewind()
{
	bRewind();
}
