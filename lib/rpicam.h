#pragma once

#ifndef DISPLAY
#define DISPLAY 0
#endif
#ifndef CAMSERVER
#define CAMSERVER 0
#endif

#include <queue>
#include <libcamera/libcamera.h>
#include <drm/drm_fourcc.h>

#if DISPLAY
#include "fbdisp.h"
#endif
#if (CAMSERVER)
#error CAMSERVER
#include "cameraserver/CameraServer.h"
#endif

#include "VisLog.h"
#include "cambase.h"
#include "frmprocessor.h"

// maximum # of managed exposure times.
#define MAX_EXPOSURES	8
#define NUM_WORKERS	4

class RPIFrame;
class RPICamera : public CamBase
{
	bool 		m_bSave = true;

	FrmProcessor	&m_nfp;
	std::mutex		m_mtxQPRF;
	std::queue<RPIFrame *> m_qprf;

	std::thread	m_grthWorkers[NUM_WORKERS];
	std::thread m_thCamera;

	std::mutex	m_queueMutex;
	std::condition_variable	m_queueCV;;

	bool		m_bShutdown;
	int			m_nCam;
	int			m_nWorkersRunning;
	int			m_nWorkersWaiting;

#if DISPLAY
	fbDisp			m_disp;
#endif
#if CAMSERVER
	cs::CvSource	m_cs;
	std::mutex		m_mtxCS;
	cs::CvSource annotatedOutput;
#endif

	std::unique_ptr<libcamera::CameraManager>			m_cm;
	std::shared_ptr<libcamera::Camera> 					m_cam;
	libcamera::FrameBufferAllocator						*m_pfba;
	std::vector<std::unique_ptr<libcamera::Request>>	m_vreq;

	std::mutex		m_mtxLog;

	uint32_t		m_u32Frames = 0;
	std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds> m_tpStart;

	//camera thread functions
	void cameraThreadFn();
	//virtual bool QueueWork( v4l2_buffer const &Buff );
	virtual bool QueueWork( libcamera::Request *request );

	void SetThreadPriority( int policy, int pri );
	void requestComplete(libcamera::Request *request);

	// Worker thread functions
	RPIFrame 	*m_pNextFrame = 0;
	RPIFrame 	*waitForWork();
	void workerThreadFn();
public:
	RPICamera( char const *pszVid, int nCam, int nRows, int nCols, uint32_t u32PixFmt, FrmProcessor &nfp );
	virtual ~RPICamera();
	virtual bool bStart();
	virtual bool bStop();

	virtual void FrameComplete( RPIFrame *pFrame );
};

