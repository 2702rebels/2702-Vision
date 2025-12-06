#pragma once
#include <cstdint>
#include <sys/time.h>

class CameraFrame
{
protected:
	int				m_nBuffer;
	int				m_nRows;
	int				m_nCols;
	uint32_t		m_u32Seq;
	uint32_t		m_u32Exp;
	uint8_t			*m_pu8Img;
	void			*m_pvData;

	struct timeval	m_ts;
public:
	CameraFrame() :
		m_pu8Img( 0 ),
		m_pvData( 0 )
	{
	}
	virtual ~CameraFrame()
	{
	}
	virtual uint32_t u32Sequence() const 
	{
		return m_u32Seq;
	}
	virtual uint32_t u32Exposure() const
	{
		return m_u32Exp;
	}
	virtual int nRows() const
	{
		return m_nRows;
	}
	virtual int nCols() const
	{
		return m_nCols;
	}
	virtual uint8_t const *pu8Image() const
	{
		return m_pu8Img;
	}
	virtual uint8_t *pu8Image()
	{
		return m_pu8Img;
	}
	virtual const struct timeval *ts() const
	{
		return &m_ts;
	}
};

class FrmProcessor
{
public:
	FrmProcessor()
	{
	}
	virtual ~FrmProcessor()
	{
	}
	virtual void ProcessFrame( int nCam, CameraFrame const *pnf )
	{
	}
};
