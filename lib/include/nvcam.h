#pragma once
#include <queue>

#include "v4lcam.h"

#include "cammgr.h"

#if DISPLAY
#include "fbdisp.h"
#endif
#if (CAMSERVER)
#error CAMSERVER
#include "cameraserver/CameraServer.h"
#endif

#include "VisLog.h"

// maximum # of managed exposure times.
#define MAX_EXPOSURES 8
#define NUM_WORKERS 4

class nvFrame {
  friend class nvCamera;
  int m_nBuffer;
  int m_nRows;
  int m_nCols;
  uint64_t m_u32Seq;
  uint64_t m_u32Exp;
  uint8_t *m_pu8Img;

  struct timeval m_ts;

public:
  nvFrame() : m_pu8Img(0) {}
  ~nvFrame() {}
  inline uint32_t u32Sequence() const { return m_u32Seq; }
  inline uint32_t u32Exposure() const { return m_u32Exp; }
  inline int nRows() const { return m_nRows; }
  inline int nCols() const { return m_nCols; }
  inline uint8_t const *pu8Image() const { return m_pu8Img; }
  inline uint8_t *pu8Image() { return m_pu8Img; }
  inline const struct timeval *ts() const { return &m_ts; }
};

class nvFrmProcessor {
public:
  nvFrmProcessor() {}
  virtual ~nvFrmProcessor() {}
  virtual void ProcessFrame(int nCam, nvFrame const *pnf) {}
};

class nvCamera : public v4lcam {
  uint64_t m_u64MaxExp;
  uint64_t m_u64NextExp;
  uint32_t m_u32PixFmt;

  bool m_bSave = true;

  nvFrmProcessor &m_nfp;

  nvFrame *m_pFrames;

  std::thread m_grthWorkers[NUM_WORKERS];
  std::thread m_thCamera;

  std::mutex m_queueMutex;
  std::condition_variable m_queueCV;
  ;

  bool m_bShutdown;
  int m_nCam;
  int m_nWorkersRunning;
  int m_nWorkersWaiting;
  nvFrame *m_pNextFrame;

#if DISPLAY
  fbDisp m_disp;
#endif
#if CAMSERVER
  cs::CvSource m_cs;
  std::mutex m_mtxCS;
  cs::CvSource annotatedOutput;
#endif

  std::mutex m_mtxLog;

  uint32_t m_u32Frames = 0;
  std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>
      m_tpStart;

public:
  nvCamera(char const *pszVid, int nCam, int nWidth, int nHeight,
           uint32_t u32PixFmt, nvFrmProcessor &nfp);
  virtual ~nvCamera();
  virtual bool bSetExposure(uint64_t uExp);
  virtual int nGetExposure();

  virtual void UpdateExposure(uint32_t u32Frame, uint64_t u64Exposure);
  virtual bool bProcessFrame(v4l2_buffer &buf);
  virtual bool bStart();
  virtual bool bStop();

  virtual void FrameComplete(nvFrame *pFrame);

  void SetThreadPriority(int policy, int pri);

  // camera thread functions
  void cameraThreadFn();
  virtual bool QueueWork(v4l2_buffer const &Buff);

  // Worker thread functions
  nvFrame *waitForWork();
  void workerThreadFn();
  void procGreenFrame(nvFrame *pFrm, cv::Mat &mat, uint8_t &u8Max);
  void procColorFrame(nvFrame *pFrm, cv::Mat &mat);
};
