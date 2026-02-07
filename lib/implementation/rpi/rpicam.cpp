#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <cinttypes>

//#include <cameraserver/CameraServer.h>

#if 0
#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#endif
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <string>
#include <stdexcept>

#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>

#define AUTO_GAIN	1
#define APRILTAGS	0
#define ANNOTATE	0

using namespace std;

#include "frmprocessor.h"
#include "cambase.h"
#include "rpicam.h"

class RPIFrame : public CameraFrame
{
	friend class RPICamera;

	libcamera::Request *m_pReq;
	uint32_t	m_u32Seq;
	uint32_t	m_u32Exposure;
	int			m_nRows = 0;
	int			m_nCols = 0;
	struct timeval m_ts;

	uint8_t	*m_pu8Img;

	void vSetFrame( libcamera::Request *pReq )
	{
		m_pReq = pReq;


		/* This code assumes that there is only 1 buffer per request, and 1 plane of data.
		 * If the camera initialization changes, a these assumptions will break.
		 */
		const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> &buffers = pReq->buffers();

		const libcamera::FrameBuffer *pBuf = m_pReq->buffers().begin()->second;
		auto rmd = pBuf->metadata();

		m_u32Seq = rmd.sequence; //pReq->sequence();
		m_ts.tv_sec = rmd.timestamp / 1000000000;
		m_ts.tv_usec = (rmd.timestamp - (m_ts.tv_sec * 1000000000)) / 1000;

		auto cookie = pBuf->cookie();

		int nWidth = (cookie >> 16) & 0xFFFF;
		int nHeight = cookie & 0xFFFF;

		libcamera::ControlList &meta = pReq->metadata();
		
		auto exp = meta.get( libcamera::controls::ExposureTime );

		m_u32Exposure = exp ? *exp : 0;

		const auto &planes = pBuf->planes();
		const auto &plane = planes[0];

		uint8_t *pu8Img = (uint8_t *)mmap( 0, plane.length, PROT_READ, MAP_PRIVATE, plane.fd.get(), plane.offset );
		if ((long)pu8Img == -1) {
			throw std::runtime_error( "mmap failed" );
		}

		if (nWidth != m_nCols || nHeight != m_nRows || !m_pu8Img) {
			delete m_pu8Img;

			m_pu8Img = new uint8_t[nWidth * nHeight * 3];
			m_nRows = nHeight;
			m_nCols = nWidth;
		}
		if (plane.length != nWidth * nHeight * 3) {
			printf( "plane.length = %d : %dx%d->%d\n", plane.length, nWidth, nHeight, nWidth * nHeight * 3 );
			throw std::runtime_error( "Plane length differnt" );
		}
		memcpy( m_pu8Img, pu8Img, nWidth * nHeight * 3 );
		if (munmap(pu8Img, plane.length) != 0) {
			throw std::runtime_error( "munmap failed" );
		}
	}

public:
	RPIFrame() :
		CameraFrame()
	{
	}
	virtual ~RPIFrame()
	{
	}
	virtual uint32_t u32Sequence() const 
	{
		return m_pReq->sequence();
	}
	virtual uint32_t u32Exposure() const
	{
		return m_u32Exposure;
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
	virtual libcamera::Request *Req() {
		return m_pReq;
	}
	
};

RPICamera::RPICamera( char const *pszVid, int nCam, int nWidth, int nHeight, uint32_t u32PixFmt, FrmProcessor &nfp ) :
	m_bShutdown( false ),
	m_nCam( nCam ),
	m_nWorkersRunning( 0 ),
	m_nWorkersWaiting( 0 ),
	m_nfp( nfp )
{
	/* SPDX-License-Identifier: GPL-2.0-or-later */
	/*
	 * Copyright (C) 2020, Ideas on Board Oy.
	 *
	 * A simple libcamera capture example
	 */
	/*
	 * --------------------------------------------------------------------
	 * Create a Camera Manager.
	 *
	 * The Camera Manager is responsible for enumerating all the Camera
	 * in the system, by associating Pipeline Handlers with media entities
	 * registered in the system.
	 *
	 * The CameraManager provides a list of available Cameras that
	 * applications can operate on.
	 *
	 * When the CameraManager is no longer to be used, it should be deleted.
	 * We use a unique_ptr here to manage the lifetime automatically during
	 * the scope of this function.
	 *
	 * There can only be a single CameraManager constructed within any
	 * process space.
	 */
	m_cm = std::make_unique<libcamera::CameraManager>();
	m_cm->start();

	/*
	 * --------------------------------------------------------------------
	 * Camera
	 *
	 * Camera are entities created by pipeline handlers, inspecting the
	 * entities registered in the system and reported to applications
	 * by the CameraManager.
	 *
	 * In general terms, a Camera corresponds to a single image source
	 * available in the system, such as an image sensor.
	 *
	 * Application lock usage of Camera by 'acquiring' them.
	 * Once done with it, application shall similarly 'release' the Camera.
	 *
	 * As an example, use the first available camera in the system after
	 * making sure that at least one camera is available.
	 *
	 * Cameras can be obtained by their ID or their index, to demonstrate
	 * this, the following code gets the ID of the first camera; then gets
	 * the camera associated with that ID (which is of course the same as
	 * cm->cameras()[0]).
	 */
	if (m_cm->cameras().empty()) {
		throw std::runtime_error( "No cameras were identified on the system" );
	}

	m_cam = m_cm->get(m_cm->cameras()[0]->id());
	m_cam->acquire();

	/*
	 * Stream
	 *
	 * Each Camera supports a variable number of Stream. A Stream is
	 * produced by processing data produced by an image source, usually
	 * by an ISP.
	 *
	 *   +-------------------------------------------------------+
	 *   | Camera                                                |
	 *   |                +-----------+                          |
	 *   | +--------+     |           |------> [  Main output  ] |
	 *   | | Image  |     |           |                          |
	 *   | |        |---->|    ISP    |------> [   Viewfinder  ] |
	 *   | | Source |     |           |                          |
	 *   | +--------+     |           |------> [ Still Capture ] |
	 *   |                +-----------+                          |
	 *   +-------------------------------------------------------+
	 *
	 * The number and capabilities of the Stream in a Camera are
	 * a platform dependent property, and it's the pipeline handler
	 * implementation that has the responsibility of correctly
	 * report them.
	 */

	/*
	 * --------------------------------------------------------------------
	 * Camera Configuration.
	 *
	 * Camera configuration is tricky! It boils down to assign resources
	 * of the system (such as DMA engines, scalers, format converters) to
	 * the different image streams an application has requested.
	 *
	 * Depending on the system characteristics, some combinations of
	 * sizes, formats and stream usages might or might not be possible.
	 *
	 * A Camera produces a CameraConfigration based on a set of intended
	 * roles for each Stream the application requires.
	 */
	std::unique_ptr<libcamera::CameraConfiguration> config =
		m_cam->generateConfiguration( { libcamera::StreamRole::Viewfinder } );

	/*
	 * The CameraConfiguration contains a StreamConfiguration instance
	 * for each StreamRole requested by the application, provided
	 * the Camera can support all of them.
	 *
	 * Each StreamConfiguration has default size and format, assigned
	 * by the Camera depending on the Role the application has requested.
	 */
	libcamera::StreamConfiguration &streamConfig = config->at(0);
	std::cout << "Default viewfinder configuration is: "
		  << streamConfig.toString() << std::endl;

	/*
	 * Each StreamConfiguration parameter which is part of a
	 * CameraConfiguration can be independently modified by the
	 * application.
	 *
	 * In order to validate the modified parameter, the CameraConfiguration
	 * should be validated -before- the CameraConfiguration gets applied
	 * to the Camera.
	 *
	 * The CameraConfiguration validation process adjusts each
	 * StreamConfiguration to a valid value.
	 */

	/*
	 * The Camera configuration procedure fails with invalid parameters.
	 */
#if 1
	streamConfig.size.width = nWidth;
	streamConfig.size.height = nHeight;
	streamConfig.pixelFormat = libcamera::PixelFormat( u32PixFmt, 0 );
	//streamConfig.pixelFormat = PixelFormat( DRM_FORMAT_BGR888, 0 );
	//streamConfig.pixelFormat = libcamera::PixelFormat( DRM_FORMAT_RGB888, 0 );
	//streamConfig.pixelFormat = PixelFormat( DRM_FORMAT_XRGB8888, 0 );
	//streamConfig.pixelFormat = PixelFormat( DRM_FORMAT_XRGB8888, 0 );

	/*
	 * Validating a CameraConfiguration -before- applying it will adjust it
	 * to a valid configuration which is as close as possible to the one
	 * requested.
	 */
	config->validate();
	std::cout << "Validated viewfinder configuration is: "
		  << streamConfig.toString() << std::endl;

	int ret = m_cam->configure(config.get());
	if (ret) {
		std::cout << "CONFIGURATION FAILED!" << std::endl;
		throw std::runtime_error( "Unable to set camera configuration" );
	}
#endif
	// Update width and height to match the actual configuration
	nWidth = streamConfig.size.width;
	nHeight = streamConfig.size.height;

	m_pfba = new libcamera::FrameBufferAllocator( m_cam );
	for (libcamera::StreamConfiguration &cfg : *config) {
		int ret = m_pfba->allocate(cfg.stream());
		if (ret < 0) {
			printf( "Unable to allocate stream buffers\n" );
			throw std::runtime_error( "Unable to allocate Stream Buffers" );
		}
	}

	/*
	 * --------------------------------------------------------------------
	 * Frame Capture
	 *
	 * libcamera frames capture model is based on the 'Request' concept.
	 * For each frame a Request has to be queued to the Camera.
	 *
	 * A Request refers to (at least one) Stream for which a Buffer that
	 * will be filled with image data shall be added to the Request.
	 *
	 * A Request is associated with a list of Controls, which are tunable
	 * parameters (similar to v4l2_controls) that have to be applied to
	 * the image.
	 *
	 * Once a request completes, all its buffers will contain image data
	 * that applications can access and for each of them a list of metadata
	 * properties that reports the capture parameters applied to the image.
	 */
	libcamera::Stream *stream = streamConfig.stream();
	const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = m_pfba->buffers(stream);
	
	for (unsigned int i = 0; i < buffers.size(); ++i) {
		std::unique_ptr<libcamera::Request> request = m_cam->createRequest();
		if (!request)
		{
			std::cerr << "Can't create request" << std::endl;
			throw std::runtime_error( "Can't create request" );
		}

		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
		// Store the width and height in the cookie!
		buffer->setCookie( ((uint32_t)nWidth) << 16 | nHeight ); 
		int ret = request->addBuffer(stream, buffer.get());
		if (ret < 0)
		{
			std::cerr << "Can't set buffer for request"
				  << std::endl;
			throw std::runtime_error( "Can't set buffer for request" );
		}

		
		/*
		 * Controls can be added to a request on a per frame basis.
		 */
		libcamera::ControlList &controls = request->controls();
//		controls.set(controls::Brightness, 0.5);


		m_vreq.push_back(std::move(request));
	}

	
	for( int i = 0; i < NUM_WORKERS; i++ )
	{
		m_grthWorkers[i] = std::thread( &RPICamera::workerThreadFn, this );
	}
	while( m_nWorkersRunning < NUM_WORKERS )
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );

	m_thCamera = std::thread( &RPICamera::cameraThreadFn, this );
}

RPICamera::~RPICamera()
{
	m_bShutdown = true;
	while( m_nWorkersRunning )
		std::this_thread::sleep_for( std::chrono::milliseconds(10) );
	
	for( int i = 0; i < NUM_WORKERS; i++ )
		m_grthWorkers[i].join();

	m_thCamera.join();

	while (m_qprf.size()) {
		delete m_qprf.front();
		m_qprf.pop();
	}
}

void RPICamera::requestComplete(libcamera::Request *request)
{
	// Check whether the frame should be requeued.
	if (QueueWork( request ) && !m_bShutdown) {
		/* Re-queue the Request to the camera. */
		request->reuse(libcamera::Request::ReuseBuffers);
		m_cam->queueRequest(request);
		printf( "ReqComplete Requeing\n" );
	}
}

#if 0
bool RPICamera::bProcessFrame( v4l2_buffer &buf )
{
	

	// If there is a worker thread waiting, queue this frame
	// Return false so buffer is not automatically re-queued
	m_u32Frames++;
	return QueueWork( buf );
}
#endif

bool RPICamera::bStart()
{
	/*
	 * Allocate a RPIFrame for each request
	 */
	for( int i = 0; i < m_vreq.size(); i++ ) {
		RPIFrame *prf = new RPIFrame();
		m_qprf.push( prf );
	}
	
	/*
	 * --------------------------------------------------------------------
	 * Signal&Slots
	 *
	 * libcamera uses a Signal&Slot based system to connect events to
	 * callback operations meant to handle them, inspired by the QT graphic
	 * toolkit.
	 *
	 * Signals are events 'emitted' by a class instance.
	 * Slots are callbacks that can be 'connected' to a Signal.
	 *
	 * A Camera exposes Signals, to report the completion of a Request and
	 * the completion of a Buffer part of a Request to support partial
	 * Request completions.
	 *
	 * In order to receive the notification for request completions,
	 * applications shall connecte a Slot to the Camera 'requestCompleted'
	 * Signal before the camera is started.
	 */
	m_cam->requestCompleted.connect(this, &RPICamera::requestComplete);

	/*
	 * --------------------------------------------------------------------
	 * Start Capture
	 *
	 * In order to capture frames the Camera has to be started and
	 * Request queued to it. Enough Request to fill the Camera pipeline
	 * depth have to be queued before the Camera start delivering frames.
	 *
	 * For each delivered frame, the Slot connected to the
	 * Camera::requestCompleted Signal is called.
	 */
	m_cam->start();
	for (std::unique_ptr<libcamera::Request> &request : m_vreq) {
		m_cam->queueRequest(request.get());
	}

	return true;
}

bool RPICamera::bStop()
{
	m_cam->stop();

	// todo: wait for all frames to be returned
	// free up frames
	return true;
}

// Processing complete for frame, re-queue buffer to the driver
void RPICamera::FrameComplete( RPIFrame *pFrame )
{
	/* Re-queue the Request to the camera. */
	auto request = pFrame->Req();

	std::unique_lock<std::mutex> l( m_mtxQPRF );
	m_qprf.push( pFrame );

	request->reuse(libcamera::Request::ReuseBuffers);
	m_cam->queueRequest(request);

}

void RPICamera::cameraThreadFn()
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
//		WaitForFrame();
		std::this_thread::sleep_for( std::chrono::milliseconds(200) );
	}

	std::chrono::duration<float> fs = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - m_tpStart);
//	std::chrono::duration_cast<std::chrono::milliseconds> tpDiff = std::chrono::steady_clock::now() - m_tpStart;
//	std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds> tpDiff = std::chrono::steady_clock::now() - m_tpStart;
	printf( "Captured %d Frames in %f seconds -- %f fps\n", m_u32Frames, fs.count(), m_u32Frames / fs.count() );
	bStop();
}

bool RPICamera::QueueWork( libcamera::Request *pReq )
{
	
	std::unique_lock<std::mutex> l( m_queueMutex );
	if( m_pNextFrame )
	{
		printf( "Discarding %" PRIu32 "\n", pReq->sequence() );
		return true;
	}

	std::unique_lock<std::mutex> lqprf( m_mtxQPRF );
	// Get pointer to frame for this request.
	RPIFrame *pFrm = m_qprf.front();
	pFrm->vSetFrame( pReq );

	if (!pFrm) {
		printf( "No Frames avialable- discarding\n" );
		return true;
	}

	m_pNextFrame = pFrm;
	m_qprf.pop();
	if( m_nWorkersWaiting )
		m_queueCV.notify_one();
	
	return false;
}

void RPICamera::SetThreadPriority( int policy, int pri )
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

RPIFrame *RPICamera::waitForWork()
{
	std::unique_lock<std::mutex> l( m_queueMutex );
	m_nWorkersWaiting++;
	m_queueCV.wait( l, [&] { return m_pNextFrame; } );

	m_nWorkersWaiting--;

	// get the pointer to the new frame to be processed
	// and reset it to 0 so no other threads try to process it
	RPIFrame *pFrm = m_pNextFrame;
	m_pNextFrame = 0;
	return pFrm;
}

void RPICamera::workerThreadFn()
{
	std::unique_lock<std::mutex> l( m_queueMutex );
	int nWorkerId = m_nWorkersRunning++;
	printf( "Worker %d started\n", m_nWorkersRunning );
	printf( "sch: %d\n", sched_getscheduler(0) );
	printf( "pri: %d\n", getpriority(PRIO_PROCESS, 0) );
	l.unlock();

	while( !m_bShutdown )
	{
		RPIFrame *pFrm = waitForWork();

		m_nfp.ProcessFrame( m_nCam, pFrm );
		FrameComplete( pFrm );
	}
	l.lock();
	printf( "Worker %d stopping\n", m_nWorkersRunning );
	m_nWorkersRunning--;
	l.unlock();
}

