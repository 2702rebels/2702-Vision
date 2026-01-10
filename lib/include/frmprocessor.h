#pragma once
#include <cstdint>
#include <chrono>

class CameraFrame
{
protected:
	int				m_nBuffer;
	int				m_nRows;
	int				m_nCols;
	uint8_t			*m_pu8Img;

	std::chrono::system_clock::time_point	m_ts;
public:
	CameraFrame() :
		m_pu8Img( 0 )
	{
	}
	virtual ~CameraFrame()
	{
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
	virtual const std::chrono::system_clock::time_point *ts() const
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
