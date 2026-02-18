#pragma once

#ifndef DISPLAY
#define DISPLAY 0
#include <mutex>
#include <thread>
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

#include "frmprocessor.h"

// maximum # of managed exposure times.
#define MAX_EXPOSURES 8
#define NUM_WORKERS 4

class CamManagerFrame : public CameraFrame {
  friend class CameraManager;
  int m_nBuffer;
  int m_nRows;
  int m_nCols;
  uint32_t m_u32Seq;
  uint32_t m_u32Exp;
  uint8_t *m_pu8Img;

  struct timeval m_ts;

public:
  CamManagerFrame() : CameraFrame(), m_pu8Img(0) {}
  virtual ~CamManagerFrame() {}
  virtual uint32_t u32Sequence() const { return m_u32Seq; }
  virtual uint32_t u32Exposure() const { return m_u32Exp; }
  virtual int nRows() const { return m_nRows; }
  virtual int nCols() const { return m_nCols; }
  virtual uint8_t const *pu8Image() const { return m_pu8Img; }
  virtual uint8_t *pu8Image() { return m_pu8Img; }
  virtual const struct timeval *ts() const { return &m_ts; }
};

class CameraManager : public v4lcam {
  bool m_bSave = true;

  FrmProcessor &m_nfp;
  CamManagerFrame *m_grpFrms;

  std::thread m_grthWorkers[NUM_WORKERS];
  std::thread m_thCamera;

  std::mutex m_queueMutex;
  std::condition_variable m_queueCV;
  ;

  bool m_bShutdown;
  int m_nCam;
  int m_nWorkersRunning;
  int m_nWorkersWaiting;
  CamManagerFrame *m_pNextFrame;

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

  // camera thread functions
  void cameraThreadFn();
  virtual bool QueueWork(v4l2_buffer const &Buff);

  void SetThreadPriority(int policy, int pri);

  // Worker thread functions
  CamManagerFrame *waitForWork();
  void workerThreadFn();

public:
  CameraManager(char const *pszVid, int nCam, int nWidth, int nHeight,
                uint32_t u32PixFmt, FrmProcessor &nfp);
  virtual ~CameraManager();
  virtual bool bProcessFrame(v4l2_buffer &buf);
  virtual bool bStart();
  virtual bool bStop();

  virtual void FrameComplete(CamManagerFrame *pFrame);

  virtual void vDoSetup(int nWidth, int nHeight, uint32_t u32PixFmt);
};
