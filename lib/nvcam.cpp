#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

//#include <cameraserver/CameraServer.h>

#include <cuda_runtime.h>

#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <string>
#include <stdexcept>

#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>

#define AUTO_GAIN	0
#define APRILTAGS	0
#define ANNOTATE	0

using namespace std;

#if 0
#define RES_ROWS	(480/2)
#define RES_COLS	(752/2)
#else
#define RES_ROWS	800
#define RES_COLS	1280
#endif

#define MAX_EXP		0x18000

#define JETSON_NANO	1
#include "nvcam.h"

nvCamera::nvCamera( char const *pszVid, int nCam, int nWidth, int nHeight, uint32_t u32PixFmt, nvFrmProcessor &nfp ) :
	v4lcam( pszVid, v4lcam::eMTUserptr ),
	m_pFrames( 0 ),
	m_bShutdown( false ),
	m_nCam( nCam ),
	m_nWorkersRunning( 0 ),
	m_nWorkersWaiting( 0 ),
	m_pNextFrame( 0 ),
	m_nfp( nfp )
{
//	printf( "Instance:: %x\n", (uint32_t)frc::CameraServer::GetInstance() );

//	bSetExposure( 0x1000 );
//	bSetSelection( 1, 5, 640, 480 );
//	bSetRes( RES_COLS, RES_ROWS, V4L2_PIX_FMT_SGRBG8 );
//	bSetRes( RES_COLS, RES_ROWS, V4L2_PIX_FMT_Y10 );
	bSetRes( RES_COLS, RES_ROWS, u32PixFmt );

	m_u64MaxExp = MAX_EXP;
	m_u64NextExp = MAX_EXP;

//	cv::namedWindow( "Img", cv::WINDOW_AUTOSIZE );

	for( int i = 0; i < NUM_WORKERS; i++ )
	{
		m_grthWorkers[i] = std::thread( &nvCamera::workerThreadFn, this );
	}
	while( m_nWorkersRunning < NUM_WORKERS )
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );

	m_thCamera = std::thread( &nvCamera::cameraThreadFn, this );
}

nvCamera::~nvCamera()
{
	m_bShutdown = true;
	while( m_nWorkersRunning )
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );
	
	for( int i = 0; i < NUM_WORKERS; i++ )
		m_grthWorkers[i].join();

	m_thCamera.join();
	
	delete [] m_pFrames;
}

/*! \brief Sets the exposure of the camera. 
* \param uExp - The new exposure of the camera. 
* \returns true if successful, false if not. 
*/
bool nvCamera::bSetExposure( uint64_t uExp )
{
	v4l2_control ctrl;

	memset( &ctrl, 0, sizeof(ctrl) );
	ctrl.id = 0x009a200a;
	ctrl.value = uExp;

	if( ioctl( m_fd, VIDIOC_S_CTRL, &ctrl ) )
		throw invalid_argument( string( "bSetExposure() failed" ) );

	return true;
}

/*! \brief Returns the exposure of the camera. */
int nvCamera::nGetExposure()
{
	v4l2_control ctrl;

	memset( &ctrl, 0, sizeof(ctrl) );
	ctrl.id = 0x009a200a;

	if( ioctl( m_fd, VIDIOC_G_CTRL, &ctrl ) )
		throw invalid_argument( string( "nvCamera::get V4L2_CID_EXPOSURE" ) );

	return ctrl.value;
}

/*! \brief Updates the exposure of the camera for the next frame.
* \bug The frame number currently has no impact, just sets for the next frame.  
* \param u32Frame - The frame number to adjust the exposure for.
* \param u64Exposure - The new exposure to use for the frame. 
*/
void nvCamera::UpdateExposure( uint32_t u32Frame, uint64_t u64Exposure )
{
	if( u64Exposure > m_u64MaxExp )
		u64Exposure = m_u64MaxExp;
	else if( !u64Exposure )
		u64Exposure = 1;

	m_u64NextExp = u64Exposure;
}

/*! \brief Process a frame from a buffer. 
* \param buf - The buffer to process the frame from.
* \returns true if successful, false if not. 
*/
bool nvCamera::bProcessFrame( v4l2_buffer &buf )
{
	printf( "Process Frame: %d/%d\n", buf.index, buf.sequence );
	//Grab some info from the buffer and store in the associted frame
	int nIdx = buf.index;
	m_pFrames[nIdx].m_u32Seq = buf.sequence;
	m_pFrames[nIdx].m_u32Exp = m_u64NextExp; // thf get actual exposure
	m_pFrames[nIdx].m_ts = buf.timestamp;
	m_pFrames[nIdx].m_pu8Img = (uint8_t *)m_grBuffers[nIdx].m_pMem;

#if !AUTO_GAIN
//	bSetExposure( m_u64NextExp );
#endif

	// If there is a worker thread waiting, queue this frame
	// Return false so buffer is not automatically re-queued
	m_u32Frames++;
	return QueueWork( buf );
}

/*! \brief Start intaking frames. 
* \returns true if successful, false if not. 
*/
bool nvCamera::bStart()
{
	m_pFrames = new nvFrame[nBuffers()];

	for( int i = 0; i < nBuffers(); i++ )
	{
		m_pFrames[i].m_nBuffer = i;
		m_pFrames[i].m_nRows = m_nRows;
		m_pFrames[i].m_nCols = m_nCols;
		m_pFrames[i].m_pu8Img = (uint8_t *)m_grBuffers[i].m_pMem;
	}

	return v4lcam::bStart();
}

/*! \brief Stop intaking frames. 
* \returns true if successful, false if not. 
*/
bool nvCamera::bStop()
{
	return v4lcam::bStop();
}

/*! \brief Triggers when processing is complete for frame, re-queue buffer to the driver.
* \param pFrame - The frame that has finished processing.
*/
void nvCamera::FrameComplete( nvFrame *pFrame )
{
	v4l2_buffer buf;

	int nBuff = pFrame->m_nBuffer;
	memset( &buf, 0, sizeof(buf) );

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if( m_eType == eMTmmap ) {
		buf.memory = V4L2_MEMORY_MMAP;
	}
	else {
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.m.userptr   = (unsigned long)m_grBuffers[nBuff].m_pMem;
		buf.length		= m_grBuffers[nBuff].m_nSize;
	}
	buf.index = nBuff;

	if( xioctl( VIDIOC_QBUF, &buf ) == -1 )
	{
		throw runtime_error( string("Failed queuing video buffer") );
	}
}

void nvCamera::cameraThreadFn()
{
	printf( "Nice: %d\n", nice(-5) );

#if AUTO_GAIN
//	bSetAutoGain( true );
//	bSetAutoExposure( true );
//	bSetAutoGain( false );
//	bSetGain( 64 );
#else
//	bSetAutoExposure( false );
//	bSetAutoGain( false );
//	bSetGain( 64 );
#endif
	m_u32Frames = 0;
	bStart();
	m_tpStart = std::chrono::steady_clock::now();


	// As long as there are workers waiting on data,
	// keep sending it to them.
	while( m_nWorkersRunning ) {
		WaitForFrame();
	}

	std::chrono::duration<float> fs = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - m_tpStart);
//	std::chrono::duration_cast<std::chrono::milliseconds> tpDiff = std::chrono::steady_clock::now() - m_tpStart;
//	std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds> tpDiff = std::chrono::steady_clock::now() - m_tpStart;
	printf( "Captured %d Frames in %f seconds -- %f fps\n", m_u32Frames, fs.count(), m_u32Frames / fs.count() );
	bStop();
}

/*! \brief Queues a frame to be processed. 
* \param buff - The frame as a buffer. 
* \returns true if successful, false if not. 
*/
bool nvCamera::QueueWork( v4l2_buffer const &buff )
{
	std::unique_lock<std::mutex> l( m_queueMutex );
	if( m_pNextFrame )
	{
		printf( "Discarding %" PRIu32 "\n", buff.sequence );
		return true;
	}

	m_pNextFrame = m_pFrames + buff.index;
	if( m_nWorkersWaiting )
		m_queueCV.notify_one();
	
	return false;
}

void nvCamera::SetThreadPriority( int policy, int pri )
{
#if 0
	sched_param parm = {
		.sched_priority = pri,
	};

	if( sched_setscheduler( 0, policy, &parm ) )
	{
		printf( "setschedparam->%d: %s\n", errno, strerror(errno) );
		throw runtime_error( string("Unable to set thread priority") );
	}
#endif
#if 0
	sched_param parm = {
		.sched_priority = pri,
	};

	if( sched_getparam( 0, &parm ) ) {
		printf( "getparam: %d:%s\n", errno, strerror(errno) );
	}
	else {
		printf( "priority: %d\n", parm. sched_priority );
	}
#endif
#if 0
	struct sched_attr attr = {
		.size = sizeof(attr),
	};

	if( sched_getattr( 0, &attr, sizeof(attr), 0 ) ) {
		printf( "getattr: %d:%s\n", errno, strerror(errno) );
	}
	else {
		printf( "size:		%d\n", attr.size );
		printf( "policy:	%d\n", attr.sched_policy );
		printf( "flags:		%x\n", attr.sched_flags );
		printf( "nice:		%d\n", attr.sched_nice );
		printf( "priority:  %d\n", attr.sched_priority );
	}

#endif
}

/*! \brief Wait for work from the manager thread. */
nvFrame *nvCamera::waitForWork()
{
	std::unique_lock<std::mutex> l( m_queueMutex );
	m_nWorkersWaiting++;
	m_queueCV.wait( l, [&] { return m_pNextFrame; } );

	m_nWorkersWaiting--;

	// get the pointer to the new frame to be processed
	// and reset it to 0 so no other threads try to process it
	nvFrame *pFrm = m_pNextFrame;
	m_pNextFrame = 0;
	return pFrm;
}

/*! \brief Function that runs on the worker threads that are processing frames.
Continously intakes frames and processes them, sending the processed frames back to the manager thread. 
*/
void nvCamera::workerThreadFn()
{
	int nRows = m_nRows/2;
	int nCols = m_nCols/2;
	cv::Mat green( nRows, nCols, CV_8UC1 );
	cv::Mat color( nRows, nCols, CV_8UC3 );
	cv::Mat blurOut;
	cv::Mat ThreshOut;

	std::unique_lock<std::mutex> l( m_queueMutex );
	int nWorkerId = m_nWorkersRunning++;
	printf( "Worker %d started\n", m_nWorkersRunning );
	printf( "sch: %d\n", sched_getscheduler(0) );
	printf( "pri: %d\n", getpriority(PRIO_PROCESS, 0) );
	l.unlock();

	while( !m_bShutdown )
	{
		nvFrame *pFrm = waitForWork();

		m_nfp.ProcessFrame( m_nCam, pFrm );
		FrameComplete( pFrm );
	}
	l.lock();
	printf( "Worker %d stopping\n", m_nWorkersRunning );
	m_nWorkersRunning--;
	l.unlock();
}

