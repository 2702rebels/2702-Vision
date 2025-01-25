#include <stdint.h>
#include <sys/time.h>

#include "LogPlayer.h"


class LogFrame : public CameraFrame
{
	cv::Mat		&m_mat;
	uint32_t	m_u32Exp;
	uint32_t	m_u32Seq;
	timeval		m_ts;
public:
	LogFrame( cv::Mat m, uint32_t u32Seq, uint32_t u32Exp ) :
		m_mat(m),
		m_u32Exp( u32Exp ),
		m_u32Seq( u32Seq )
	{
		gettimeofday( &m_ts, 0 );
	}
	~LogFrame();

	virtual uint32_t u32Sequence() const {
		return m_u32Seq;
	}
	virtual uint32_t u32Exposure() const {
		return m_u32Exp;
	}
	virtual int nRows() const {
		return m_mat.rows;
	}
	virtual int nCols() const {
		return m_mat.cols;
	}
	virtual uint8_t const *pu8Image() const
	{
		return m_mat.ptr();
	}
	virtual uint8_t *pu8Image()
	{
		return m_mat.ptr();
	}
	virtual const struct timeval *ts() const
	{
		return &m_ts;
	}
};

LogPlayer::LogPlayer( char const *psz, FrmProcessor &fp ) :
	VisionLog( psz, false ),
	m_fp( fp )
{
}

LogPlayer::~LogPlayer()
{
}

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
