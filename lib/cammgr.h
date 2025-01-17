#pragma once

#ifndef DISPLAY
#define DISPLAY 0
#endif
#ifndef CAMSERVER
#define CAMSERVER 0
#endif

#include "v4lcam.h"

#if DISPLAY
#include "fbdisp.h"
#endif
#if (CAMSERVER)
#error CAMSERVER
#include "cameraserver/CameraServer.h"
#endif

#include "VisLog.h"

// maximum # of managed exposure times.
#define MAX_EXPOSURES	8
#define NUM_WORKERS	4

class CameraFrame
{
	friend class CameraManager;
	int				m_nBuffer;
	int				m_nRows;
	int				m_nCols;
	uint32_t		m_u32Seq;
	uint32_t		m_u32Exp;
	uint8_t			*m_pu8Img;

	struct timeval	m_ts;
public:
	CameraFrame() :
		m_pu8Img( 0 )
	{
	}
	~CameraFrame()
	{
	}
	inline uint32_t u32Sequence() const 
	{
		return m_u32Seq;
	}
	inline uint32_t u32Exposure() const
	{
		return m_u32Exp;
	}
	inline int nRows() const
	{
		return m_nRows;
	}
	inline int nCols() const
	{
		return m_nCols;
	}
	inline uint8_t const *pu8Image() const
	{
		return m_pu8Img;
	}
	inline uint8_t *pu8Image()
	{
		return m_pu8Img;
	}
	inline const struct timeval *ts() const
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

class CameraManager : public v4lcam
{
	int			m_nExposures;
	uint64_t	m_u32MaxExp;
	uint64_t	m_gruCurExp[MAX_EXPOSURES];
	uint64_t	m_gruNextExp[MAX_EXPOSURES];
	bool 		m_bSave = true;

	FrmProcessor	&m_nfp;
	CameraFrame		*m_grpFrms;

	std::thread	m_grthWorkers[NUM_WORKERS];
	std::thread m_thCamera;

	std::mutex	m_queueMutex;
	std::condition_variable	m_queueCV;;

	bool		m_bShutdown;
	int			m_nCam;
	int			m_nWorkersRunning;
	int			m_nWorkersWaiting;
	CameraFrame		*m_pNextFrame;

#if DISPLAY
	fbDisp			m_disp;
#endif
#if CAMSERVER
	cs::CvSource	m_cs;
	std::mutex		m_mtxCS;
	cs::CvSource annotatedOutput;
#endif

	std::mutex		m_mtxLog;

	uint32_t		m_u32Frames = 0;
	std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds> m_tpStart;
public:
	CameraManager( char const *pszVid, int nExposures, int nCam, FrmProcessor &nfp );
	virtual ~CameraManager();
	virtual void UpdateExposure( uint32_t u32Frame, uint32_t u32Exposure );
	virtual bool bProcessFrame( v4l2_buffer &buf );
	virtual bool bStart();
	virtual bool bStop();

	virtual void FrameComplete( CameraFrame *pFrame );

	void SetThreadPriority( int policy, int pri );

	//camera thread functions
	void cameraThreadFn();
	virtual bool QueueWork( v4l2_buffer const &Buff );


	// Worker thread functions
	CameraFrame *waitForWork();
	void workerThreadFn();
	void procGreenFrame( CameraFrame *pFrm, cv::Mat &mat, uint8_t &u8Max );
	void procColorFrame( CameraFrame *pFrm, cv::Mat &mat );
};

