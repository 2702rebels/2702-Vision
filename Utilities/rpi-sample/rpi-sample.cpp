#include <errno.h>
#include <fcntl.h>
#include <span>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// #include <X11/Xlib.h>
#include <condition_variable>
#include <linux/videodev2.h>
#include <mutex>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <thread>
#include <vector>

#include <stdexcept>
#include <string>

#include <cameraserver/CameraServer.h>
#include <drm/drm_fourcc.h>
#include <networktables/NetworkTableInstance.h>

#include "VisionLog.h"
#include "rpicam.h"

using namespace std;

#include <signal.h>

bool bStop = false;

void sighandler(int nSig) {
  printf("Received Signal %d\n", nSig);
  bStop = true;
}

class imgProc : public FrmProcessor {
  VisionLogWriter m_vl;
  std::mutex m_mtxLog;
  std::mutex m_mtxCS;
  bool m_bFirstVid = true;
  cs::CvSource m_outputStream;

public:
  imgProc(char const *pszLogFile)
      : FrmProcessor(), m_vl(fs::path(pszLogFile), LogFrameType::RGB, 640, 480)

  {}
  virtual ~imgProc() {}
  virtual void ProcessFrame(int nCam, CameraFrame const *pcf) {
    cv::Mat frame(pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image());
    //		printf( "Have Frame from %d:%d\n", nCam, pcf->u32Sequence() );

    if (m_mtxLog.try_lock()) {
      m_vl.WriteFrame(frame);
      m_mtxLog.unlock();
    }
    if (m_mtxCS.try_lock()) {
      if (m_bFirstVid) {
        m_outputStream =
            frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows());
        m_bFirstVid = false;
      }
      m_outputStream.PutFrame(frame);
      m_mtxCS.unlock();
    }
  }
};

int main(int argc, char **argv) {
  char szLog[1024] = "test.log";
  if (access("/data/logs", F_OK) != 0) {
    throw;
  }
  for (int i = 0; i < 1024; i++) {
    sprintf(szLog, "/data/logs/test%05d.log", i);
    if (access(szLog, F_OK) != 0)
      break;
  }
  printf("Logging to %s\n", szLog);

  signal(SIGINT, sighandler);
  if (argc == 2) {
    strcpy(szLog, argv[1]);
  }
  try {
    auto ntinst = nt::NetworkTableInstance::GetDefault();
    ntinst.StartClient4("2702");
    ntinst.StartServer();
    imgProc proc(szLog);
    //		RPICamera mycam0( "/dev/video0", 0, 800, 600,
    // V4L2_PIX_FMT_RGB24, proc );
    RPICamera mycam0("/dev/video0", 0, 640, 480, DRM_FORMAT_RGB888, proc);

    while (!bStop) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  } catch (std::exception &e) {
    printf("Exception: %s\n", e.what());
  }

  return 0;
}
