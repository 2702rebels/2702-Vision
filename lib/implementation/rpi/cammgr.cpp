#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// #include <cameraserver/CameraServer.h>

#include <condition_variable>
#include <linux/videodev2.h>
#include <mutex>
#include <opencv2/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <thread>
#include <vector>

#include <stdexcept>
#include <string>

#include <sched.h>
#include <sys/resource.h>
#include <sys/time.h>

#define AUTO_GAIN 1
#define APRILTAGS 0
#define ANNOTATE 0

using namespace std;

#include "cammgr.h"

CameraManager::CameraManager(char const *pszVid, int nCam, int nWidth,
                             int nHeight, uint32_t u32PixFmt, FrmProcessor &nfp)
    : v4lcam(pszVid), m_grpFrms(0), m_bShutdown(false), m_nCam(nCam),
      m_nWorkersRunning(0), m_nWorkersWaiting(0), m_pNextFrame(0), m_nfp(nfp) {
  vDoSetup(nWidth, nHeight, u32PixFmt);

  for (int i = 0; i < NUM_WORKERS; i++) {
    m_grthWorkers[i] = std::thread(&CameraManager::workerThreadFn, this);
  }
  while (m_nWorkersRunning < NUM_WORKERS)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  m_thCamera = std::thread(&CameraManager::cameraThreadFn, this);
}

CameraManager::~CameraManager() {
  m_bShutdown = true;
  while (m_nWorkersRunning)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  for (int i = 0; i < NUM_WORKERS; i++)
    m_grthWorkers[i].join();

  m_thCamera.join();

  delete[] m_grpFrms;
}

void CameraManager::vDoSetup(int nWidth, int nHeight, uint32_t u32PixFmt) {
  if (!bSetRes(nWidth, nHeight, u32PixFmt)) {
    throw runtime_error(string("Failed Setting video resolution"));
  }
}

bool CameraManager::bProcessFrame(v4l2_buffer &buf) {
  // Grab some info from the buffer and store in the associted frame
  int nIdx = buf.index;
  m_grpFrms[nIdx].m_u32Seq = buf.sequence;
  m_grpFrms[nIdx].m_u32Exp = nGetExposure();
  m_grpFrms[nIdx].m_ts = buf.timestamp;
  m_grpFrms[nIdx].m_pu8Img = (uint8_t *)m_grBuffers[nIdx].m_pMem;

  // If there is a worker thread waiting, queue this frame
  // Return false so buffer is not automatically re-queued
  m_u32Frames++;
  return QueueWork(buf);
}

bool CameraManager::bStart() {
  m_grpFrms = new CamManagerFrame[nBuffers()];

  for (int i = 0; i < nBuffers(); i++) {
    m_grpFrms[i].m_nBuffer = i;
    m_grpFrms[i].m_nRows = m_nRows;
    m_grpFrms[i].m_nCols = m_nCols;
    m_grpFrms[i].m_pu8Img = (uint8_t *)m_grBuffers[i].m_pMem;
  }

  return v4lcam::bStart();
}

bool CameraManager::bStop() { return v4lcam::bStop(); }

// Processing complete for frame, re-queue buffer to the driver
void CameraManager::FrameComplete(CamManagerFrame *pFrame) {
  v4l2_buffer buf;

  int nBuff = pFrame->m_nBuffer;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = nBuff;

  if (xioctl(VIDIOC_QBUF, &buf) == -1) {
    throw runtime_error(string("Failed queuing video buffer"));
  }
}

void CameraManager::cameraThreadFn() {
  printf("Nice: %d\n", nice(-5));

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
  while (m_nWorkersRunning) {
    WaitForFrame();
  }

  std::chrono::duration<float> fs =
      std::chrono::duration_cast<std::chrono::duration<float>>(
          std::chrono::steady_clock::now() - m_tpStart);
  //	std::chrono::duration_cast<std::chrono::milliseconds> tpDiff =
  // std::chrono::steady_clock::now() - m_tpStart;
  //	std::chrono::time_point<std::chrono::steady_clock,std::chrono::nanoseconds>
  // tpDiff = std::chrono::steady_clock::now() - m_tpStart;
  printf("Captured %d Frames in %f seconds -- %f fps\n", m_u32Frames,
         fs.count(), m_u32Frames / fs.count());
  bStop();
}

bool CameraManager::QueueWork(v4l2_buffer const &buff) {
  std::unique_lock<std::mutex> l(m_queueMutex);
  if (m_pNextFrame) {
    printf("Discarding %" PRIu32 "\n", buff.sequence);
    return true;
  }

  m_pNextFrame = m_grpFrms + buff.index;
  if (m_nWorkersWaiting)
    m_queueCV.notify_one();

  return false;
}

void CameraManager::SetThreadPriority(int policy, int pri) {
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

CamManagerFrame *CameraManager::waitForWork() {
  std::unique_lock<std::mutex> l(m_queueMutex);
  m_nWorkersWaiting++;
  m_queueCV.wait(l, [&] { return m_pNextFrame; });

  m_nWorkersWaiting--;

  // get the pointer to the new frame to be processed
  // and reset it to 0 so no other threads try to process it
  CamManagerFrame *pFrm = m_pNextFrame;
  m_pNextFrame = 0;
  return pFrm;
}

void CameraManager::workerThreadFn() {
  std::unique_lock<std::mutex> l(m_queueMutex);
  int nWorkerId = m_nWorkersRunning++;
  printf("Worker %d started\n", m_nWorkersRunning);
  printf("sch: %d\n", sched_getscheduler(0));
  printf("pri: %d\n", getpriority(PRIO_PROCESS, 0));
  l.unlock();

  while (!m_bShutdown) {
    CamManagerFrame *pFrm = waitForWork();

    m_nfp.ProcessFrame(m_nCam, pFrm);
    FrameComplete(pFrm);
  }
  l.lock();
  printf("Worker %d stopping\n", m_nWorkersRunning);
  m_nWorkersRunning--;
  l.unlock();
}
