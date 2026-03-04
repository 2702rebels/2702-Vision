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

#define TEGRA_CAMERA_CID_BASE   (V4L2_CTRL_CLASS_CAMERA | 0x2000)

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
	bSetMode( 3 );

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

int nvCamera::nCam()
{
	return m_nCam;
}

bool nvCamera::bSetExposure( uint64_t uExp )
{
	v4l2_ext_control ctrl;
	v4l2_ext_controls ctrls;

	memset( &ctrl, 0, sizeof(ctrl) );
	memset( &ctrls, 0, sizeof(ctrls) );
	ctrl.id = 0x009a200a;
//	ctrl.id = V4L2_CID_EXPOSURE;
	ctrl.value64 = uExp;

	ctrls.count = 1;
	ctrls.controls = &ctrl;

	if( ioctl( m_fd, VIDIOC_S_EXT_CTRLS, &ctrls ) ) {
		char szTmp[1024];
		sprintf( szTmp, "bSetExposure(%lu) failed: %d\n", uExp, errno );
		throw invalid_argument( szTmp );
	}

	return true;
}

int nvCamera::nGetExposure()
{
	v4l2_ext_control ctrl;
	v4l2_ext_controls ctrls;

	memset( &ctrl, 0, sizeof(ctrl) );
	ctrl.id = 0x009a200a;

	ctrls.which = V4L2_CTRL_WHICH_CUR_VAL;
	ctrls.count = 1;
	ctrls.controls = &ctrl;

	if( ioctl( m_fd, VIDIOC_G_EXT_CTRLS, &ctrls ) )
		throw invalid_argument( string( "nvCamera::get V4L2_CID_EXPOSURE" ) );

	return ctrl.value64;
}

uint64_t nvCamera::u64GetExposure()
{
	v4l2_ext_control ctrl;
	v4l2_ext_controls ctrls;

	memset( &ctrl, 0, sizeof(ctrl) );
	ctrl.id = 0x009a200a;

	ctrls.which = V4L2_CTRL_WHICH_CUR_VAL;
	ctrls.count = 1;
	ctrls.controls = &ctrl;

	if( ioctl( m_fd, VIDIOC_G_EXT_CTRLS, &ctrls ) )
		throw invalid_argument( string( "nvCamera::get V4L2_CID_EXPOSURE" ) );

	return ctrl.value64;
}

bool nvCamera::bSetGain( uint32_t uGain )
{
	v4l2_ext_control ctrl;
	v4l2_ext_controls ctrls;

	memset( &ctrl, 0, sizeof(ctrl) );
	memset( &ctrls, 0, sizeof(ctrls) );
	ctrl.id = 0x009a2009;
	ctrl.value64 = uGain;
	printf( "Setting Gain to %u\n", uGain );

	ctrls.count = 1;
	ctrls.controls = &ctrl;

	if( ioctl( m_fd, VIDIOC_S_EXT_CTRLS, &ctrls ) ) {
		char szTmp[1024];
		sprintf( szTmp, "bSetGain(%u) failed: %d\n", uGain, errno );
		throw invalid_argument( szTmp );
	}

	return true;
}

bool nvCamera::bSetMode( uint32_t u32Mode )
{
	v4l2_ext_control ctrl;
	v4l2_ext_controls ctrls;

	memset( &ctrl, 0, sizeof(ctrl) );
	memset( &ctrls, 0, sizeof(ctrls) );
	//ctrl.id = 0x009a2009;
	//ctrl.id = TEGRA_CAMERA_CID_BASE + 8;
	ctrl.id = 0x009a3000;
	ctrl.value64 = u32Mode;
	printf( "Setting Sensor Mode to %u\n", u32Mode );

	ctrls.count = 1;
	ctrls.controls = &ctrl;

	if( ioctl( m_fd, VIDIOC_S_EXT_CTRLS, &ctrls ) ) {
		char szTmp[1024];
		sprintf( szTmp, "bSetMode(%u) failed: %d\n", u32Mode, errno );
		throw invalid_argument( szTmp );
	}

	return true;
}

void nvCamera::UpdateExposure( uint32_t u32Frame, uint64_t u64Exposure )
{
	if( u64Exposure > m_u64MaxExp )
		u64Exposure = m_u64MaxExp;
	else if( !u64Exposure )
		u64Exposure = 1;

	m_u64NextExp = u64Exposure;
}

static uint64_t u64OffsetNs()
{
	uint64_t	u64RawNsec;
	uint64_t	u64TscNs;
	uint64_t	u64Cycles;
	uint64_t	u64Freq;
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	asm volatile("mrs %0, cntfrq_el0" : "=r"(u64Freq));
	asm volatile("mrs %0, cntvct_el0" : "=r"(u64Cycles));

	u64TscNs = u64Cycles / u64Freq * 1000000000;
	u64RawNsec = tp.tv_sec * 1000000000 + tp.tv_nsec;

	uint64_t u64Offset = llabs(u64TscNs - u64RawNsec);

	return u64Offset;
}


bool nvCamera::bProcessFrame( v4l2_buffer &buf )
{
//	printf( "Process Frame: %d/%d\n", buf.index, buf.sequence );
	//Grab some info from the buffer and store in the associted frame
	int nIdx = buf.index;
	m_pFrames[nIdx].m_u32Seq = buf.sequence;
	m_pFrames[nIdx].m_u32Exp = m_u64NextExp; // thf get actual exposure
#if 0
	m_pFrames[nIdx].m_ts = buf.timestamp;
	uint64_t u64Off = u64OffsetNs();

	struct timespec tsCur;

	// Get current time since power on
	clock_gettime(CLOCK_MONOTONIC_RAW, &tsCur );
//	printf( "%lu.%09lu  -> %lu.%06lu   %lu\n", tsCur.tv_sec, tsCur.tv_nsec, buf.timestamp.tv_sec, buf.timestamp.tv_usec, u64Off );
	
	uint64_t u64TS = buf.timestamp.tv_sec * 1000000 + buf.timestamp.tv_usec - u64Off / 1000;
	
	m_pFrames[nIdx].m_ts.tv_sec = u64TS / 1000000;
	m_pFrames[nIdx].m_ts.tv_usec = (u64TS - m_pFrames[nIdx].m_ts.tv_sec * 1000000);
	printf( "%d: %lu.%09lu  -> %lu.%06lu  ->  %lu.%06lu ->  %lu\n", m_nCam, tsCur.tv_sec, tsCur.tv_nsec, buf.timestamp.tv_sec, buf.timestamp.tv_usec, m_pFrames[nIdx].m_ts.tv_sec, m_pFrames[nIdx].m_ts.tv_usec, u64Off );
#else
	struct timespec tsCur;

	// Get current time since power on
	clock_gettime(CLOCK_MONOTONIC_RAW, &tsCur );
//clock_gettime(CLOCK_MONOTONIC, &tsCur );
	TIMESPEC_TO_TIMEVAL( &m_pFrames[nIdx].m_ts, &tsCur );
#endif
//printf( "Have Frame %d:%u : %ld.%06ld\n", nCam(), buf.sequence, buf.timestamp.tv_sec, buf.timestamp.tv_usec );
//printf( "                   %ld.%06ld\n", m_pFrames[nIdx].m_ts.tv_sec, m_pFrames[nIdx].m_ts.tv_usec );
//printf( "                   %ld\n", buf.timestamp.tv_sec - m_pFrames[nIdx].m_ts.tv_sec );

	m_pFrames[nIdx].m_pu8Img = (uint8_t *)m_grBuffers[nIdx].m_pMem;

#if !AUTO_GAIN
//	bSetExposure( m_u64NextExp );
#endif

	// If there is a worker thread waiting, queue this frame
	// Return false so buffer is not automatically re-queued
	m_u32Frames++;
	return QueueWork( buf );
}

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

bool nvCamera::bStop()
{
	return v4lcam::bStop();
}

// Processing complete for frame, re-queue buffer to the driver
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

		m_nfp.ProcessFrame( this, pFrm );
		FrameComplete( pFrm );
	}
	l.lock();
	printf( "Worker %d stopping\n", m_nWorkersRunning );
	m_nWorkersRunning--;
	l.unlock();
}

