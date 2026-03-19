#pragma once
#include <vector>

#include <opencv2/core/core.hpp>

class ATDetection
{
	unsigned					m_uId;
	std::vector<cv::Point2f>	m_vxCorners;
	bool						m_bUse;				// true if Apriltag is usable.
public:
	ATDetection( unsigned uId, double grCrnrs[4][2] ) :
		m_uId( uId ),
		m_bUse( true )
	{
		m_uId = uId;
		for( int i = 0; i < 4; i++ )
		{
			m_vxCorners.push_back( cv::Point2f( grCrnrs[i][0], grCrnrs[i][1] ) );
		}
	}
	ATDetection( const ATDetection &src ) :
		m_uId( src.m_uId ),
		m_vxCorners( src.m_vxCorners ),
		m_bUse( src.m_bUse )
	{
	}
	~ATDetection()
	{
	}
	unsigned uId() const
	{
		return m_uId;
	}

	ATDetection &operator=( ATDetection const &src )
	{
		m_uId = src.m_uId;
		m_vxCorners = src.m_vxCorners;
		m_bUse = src.m_bUse;

		return *this;
	}

	cv::Point2f const &operator[](int n) const
	{
		return m_vxCorners[n];
	}

	void vSetUse( bool bUse )
	{
		m_bUse = bUse;
	}

	bool bUse() const
	{
		return m_bUse;
	}
};

class ATFrame
{
	int							m_nCam;
	uint32_t					m_u32Seq;
	struct timeval				m_ts;
	std::vector<ATDetection>	m_vxTags;
public:
	ATFrame( int nCam, uint32_t u32Seq, timeval const *ts ) :
		m_nCam( nCam ),
		m_u32Seq( u32Seq )
	{
		m_ts.tv_sec = ts->tv_sec;
		m_ts.tv_usec = ts->tv_usec;
	}
	~ATFrame()
	{
	}

	int	nCam() const 
	{
		return m_nCam;
	}

	uint32_t u32Seq() const
	{
		return m_u32Seq;
	}

	timeval const &ts() const
	{
		return m_ts;
	}

	size_t sTags() const
	{
		return m_vxTags.size();
	}

	ATDetection const &operator[]( int n ) const
	{
		return m_vxTags[n];
	}

	ATDetection &operator[]( int n )
	{
		return m_vxTags[n];
	}

	void vAddDetection( ATDetection &atd )
	{
		m_vxTags.push_back( atd );
	}
};

