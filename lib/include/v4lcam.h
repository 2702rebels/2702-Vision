#ifndef V4L_CAMERA_H
#define V4L_CAMERA_H

#include <fcntl.h>
#include <inttypes.h>
#include <malloc.h>

class v4lImageProcessor {
public:
  v4lImageProcessor() {}
  virtual ~v4lImageProcessor() {}
  virtual bool bProcessImage(v4l2_buffer &buf) { return true; }
};

class v4lcam {
public:
  enum eMemType { eMTNone, eMTUserptr, eMTmmap };

protected:
  eMemType m_eType;
  bool m_bStreaming;
  int m_fd;
  int m_nPlanes;
  timeval m_tvLast;

  uint32_t m_u32PixFmt;
  int m_nRows;
  int m_nCols;
  int m_nImgSize;
  int m_nBuffers;

  int m_nExposure = -1;

  v4lImageProcessor *m_pProc = 0;

  struct memBuff {
    eMemType m_eType;
    size_t m_nSize;
    void *m_pMem;

    memBuff() : m_nSize(0), m_pMem(0), m_eType(eMTNone) {}
    ~memBuff() { free(); }
    bool mapm(int fd, off_t nOffset, size_t nLen) {
      m_eType = eMTNone;
      m_nSize = nLen;
      m_pMem = mmap(0, nLen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, nOffset);

      if (MAP_FAILED == m_pMem) {
        return false;
      }
      return true;
    }
    bool alloc(size_t nLen) {
      size_t sPage = getpagesize();
      size_t sBuffer = (nLen + sPage - 1) & ~(sPage - 1);
#if (JETSON_NANO)
      cudaMallocManaged(&m_pMem, sBuffer, cudaMemAttachGlobal);
#else
      m_pMem = memalign(sPage, sBuffer);
#endif
      m_nSize = sBuffer;
      return true;
    }
    void free() {
      if (m_eType == eMTmmap) {
        munmap(m_pMem, m_nSize);
      } else {
#if (JETSON_NANO)
        cudaFree(m_pMem);
#else
        ::free(m_pMem);
#endif
      }
      m_pMem = 0;
      m_nSize = 0;
    }
    inline size_t len() const { return m_nSize; }
    inline uint8_t *buf() { return (uint8_t *)m_pMem; }
  };

  memBuff m_grBuffers[12];

  int xioctl(int req, void *pArg) {
    int res;

    do {
      res = ioctl(m_fd, req, pArg);
    } while (-1 == res && EINTR == errno);

    return res;
  }
  virtual void initMem() {
    v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));

    req.count = m_nBuffers;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = (m_eType == eMTmmap) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
      if (EINVAL == errno)
        throw invalid_argument(
            string("video device does not support requested memory type"));

      printf("initMem()\n");
      throw invalid_argument(string(strerror(errno)));
    }

    if (req.count != m_nBuffers)
      throw invalid_argument(string("insufficient buffers"));

    if (m_eType == eMTmmap) {
      for (int i = 0; i < req.count; ++i) {
        v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(VIDIOC_QUERYBUF, &buf))
          throw invalid_argument("Failed to query buffer");

        if (!m_grBuffers[i].mapm(m_fd, buf.m.offset, buf.length))
          throw runtime_error("Failed mapping buffer");
      }
    } else {
      for (int i = 0; i < req.count; ++i) {
        if (!m_grBuffers[i].alloc(m_nImgSize))
          throw runtime_error("Failed allocating vidoe buffer");
      }
    }
  }
  virtual void freeMem() {
    for (int i = 0; i < m_nBuffers; i++) {
      m_grBuffers[i].free();
    }
  }

public:
  v4lcam(char const *psz, eMemType eType = eMTmmap)
      : m_eType(eType), m_bStreaming(false), m_nPlanes(0) {
    m_fd = open(psz, O_RDWR);
    if (m_fd < 0)
      throw invalid_argument(string("invalid file"));

    v4l2_capability cap;
    if (-1 == xioctl(VIDIOC_QUERYCAP, &cap))
      throw invalid_argument(string("invalid file"));

    if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE))
      throw invalid_argument(string("dev not capable of VIDEO Capture"));

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
      throw invalid_argument(string("dev not capable of streaming"));

    m_nBuffers = sizeof(m_grBuffers) / sizeof(m_grBuffers[0]);
  }
  virtual ~v4lcam() {
    v4l2_requestbuffers req;

    if (m_bStreaming)
      bStop();

    for (int i = 0; i < m_nBuffers; i++) {
      m_grBuffers[i].free();
    }

    memset(&req, 0, sizeof(req));

    req.count = 0;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
      if (EINVAL == errno) {
        printf("~v4lcam() EINVAL!!\n");
        //				throw invalid_argument( string("video
        //device does not support memory mapping" ) );
      }

      printf("~v4lcam() clearing buffers\n");
      //			throw invalid_argument( string(strerror(errno))
      //);
    }

    if (m_fd >= 0)
      close(m_fd);
  }
  virtual void vSetProcessor(v4lImageProcessor *pProc) { m_pProc = pProc; }
  virtual bool bProcessFrame(v4l2_buffer &buf) {
    if (m_pProc) {
      return m_pProc->bProcessImage(buf);
    } else {
      printf("Frame %" PRIu32 " Idx = %d\n", buf.sequence, buf.index);
    }
    // flag to requeue the buffer
    return true;
  }
  virtual bool flipVert(bool bFlip) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_VFLIP;

    ctrl.value = bFlip ? 1 : 0;
    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
      throw invalid_argument(string("V4L2_CID_VFLIP"));
    return true;
  }
  virtual bool flipHoriz(bool bFlip) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_HFLIP;

    ctrl.value = bFlip ? 1 : 0;
    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
      throw invalid_argument(string("V4L2_CID_HFLIP"));
    return true;
  }

  virtual bool bSetCrop(int nWidth, int nHeight) {
    v4l2_cropcap cropcap;
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (!xioctl(VIDIOC_CROPCAP, &cropcap)) {
      v4l2_crop crop;

      crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      crop.c = cropcap.defrect;

      if (-1 == xioctl(VIDIOC_S_CROP, &crop)) {
        switch (errno) {
        case EINVAL:
          // cropping not supported
          break;
        default:
          break;
        };
      }
    }

    return true;
  }
  virtual bool bSetRes(int nWidth, int nHeight, uint32_t u32PixFmt) {
    v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    fmt.fmt.pix_mp.width = nWidth;
    fmt.fmt.pix_mp.height = nHeight;
    fmt.fmt.pix_mp.pixelformat = u32PixFmt;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;

    if (-1 == xioctl(VIDIOC_S_FMT, &fmt)) {
      printf("Error = %s (%d)\n", strerror(errno), errno);
      throw invalid_argument(string("Invalid video format"));
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(VIDIOC_G_FMT, &fmt)) {
      printf("Error = %s (%d)\n", strerror(errno), errno);
      throw invalid_argument(string("failed requesting video format"));
    }

    m_nRows = nHeight;
    m_nCols = nWidth;
    m_nImgSize = fmt.fmt.pix.sizeimage;
    m_u32PixFmt = u32PixFmt;

    return true;
  }
  virtual bool bStart() {
    m_tvLast.tv_sec = 0;
    m_tvLast.tv_usec = 0;

    initMem();

    for (int i = 0; i < sizeof(m_grBuffers) / sizeof(m_grBuffers[0]); i++) {
      v4l2_buffer buf;

      memset(&buf, 0, sizeof(buf));
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.index = i;
      if (m_eType == eMTmmap) {
        buf.memory = V4L2_MEMORY_MMAP;
      } else {
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.m.userptr = (unsigned long)m_grBuffers[i].m_pMem;
        buf.length = m_grBuffers[i].m_nSize;
      }

      if (xioctl(VIDIOC_QBUF, &buf) == -1) {
        throw runtime_error(string("Failed queuing video buffer"));
      }
    }
    v4l2_buf_type typ = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(VIDIOC_STREAMON, &typ) == -1) {
      printf("STREAMON: %s\n", strerror(errno));
      throw runtime_error(string("failed to start streaming"));
    }

    m_bStreaming = true;
    return true;
  }
  virtual void WaitForFrame() {
    timeval tv;

    v4l2_buffer buf;

    do {
      memset(&buf, 0, sizeof(buf));
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (m_eType == eMTmmap) {
        buf.memory = V4L2_MEMORY_MMAP;
      } else {
        buf.memory = V4L2_MEMORY_USERPTR;
      }

      if (xioctl(VIDIOC_DQBUF, &buf) == 0) {
        printf("Buf: %d\n", buf.index);
        // process this frame...
        if (bProcessFrame(buf)) {
          // re-queue the buffer
          if (xioctl(VIDIOC_QBUF, &buf) == 0)
            return;
          throw runtime_error(string("failed to queue buffer"));
        }
      } else if (errno != EAGAIN) {
        char szBuff[256];
        sprintf(szBuff, "failed to get buffer (%d): %s", errno,
                strerror(errno));
        throw runtime_error(string(szBuff));
      }

    } while (1);
  }
  virtual bool bStop() {
    v4l2_buf_type typ = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(VIDIOC_STREAMOFF, &typ) == -1)
      throw runtime_error(string("failed to stop streaming"));

    return true;
  }
  virtual bool bSetAutoExposure(bool bOn) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = bOn ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
      throw invalid_argument(string("V4L2_CID_EXPOSURE_AUTO"));

    return true;
  }
  virtual bool bSetExposure(uint64_t uExp) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_EXPOSURE;
    ctrl.value = uExp;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
      throw invalid_argument(string("set V4L2_CID_EXPOSURE"));

    return true;
  }
  virtual int nGetExposure() {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_EXPOSURE;

    if (ioctl(m_fd, VIDIOC_G_CTRL, &ctrl))
      throw invalid_argument(string("get V4L2_CID_EXPOSURE"));

    return ctrl.value;
  }
  virtual bool bSetAutoGain(bool bOn) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_AUTOGAIN;
    ctrl.value = bOn ? 1 : 0;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
      throw invalid_argument(string("V4L2_CID_AUTOGAIN"));

    return true;
  }
  virtual bool bSetGain(uint32_t uGain) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_GAIN;
    ctrl.value = uGain;

    if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
      throw invalid_argument(string("V4L2_CID_GAIN"));

    return true;
  }
  virtual int nGetGain() {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));

    ctrl.id = V4L2_CID_GAIN;

    if (ioctl(m_fd, VIDIOC_G_CTRL, &ctrl))
      throw invalid_argument(string("V4L2_CID_GAIN"));

    return ctrl.value;
  }
  virtual bool bSetISO(uint32_t uISO) {
    v4l2_control ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    if (uISO == 0) {
      ctrl.id = V4L2_CID_ISO_SENSITIVITY_AUTO;
      ctrl.value = V4L2_ISO_SENSITIVITY_AUTO;

      if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
        throw invalid_argument(string("V4L2_CID_ISO_SENSITIVITY_AUTO"));
    } else {
      ctrl.id = V4L2_CID_ISO_SENSITIVITY_AUTO;
      ctrl.value = V4L2_ISO_SENSITIVITY_MANUAL;

      if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
        throw invalid_argument(string("V4L2_CID_ISO_SENSITIVITY_AUTO"));

      ctrl.id = V4L2_CID_ISO_SENSITIVITY;
      ctrl.value = uISO;

      if (ioctl(m_fd, VIDIOC_S_CTRL, &ctrl))
        throw invalid_argument(string("V4L2_CID_ISO_SENSITIVITY"));
    }
    return true;
  }
  virtual bool bSetSelection(uint16_t left, uint16_t top, uint16_t width,
                             uint16_t height) {
    struct v4l2_selection sel = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .target = V4L2_SEL_TGT_CROP,
        .flags = 0,
    };

    sel.r.left = left;
    sel.r.top = top;
    sel.r.width = width;
    sel.r.height = height;

    if (ioctl(m_fd, VIDIOC_S_SELECTION, &sel)) {
      printf("Failed S_SELECTION: %s (%d)\n", strerror(errno), errno);
      throw invalid_argument(string("VIDIOC_S_SELECTION"));
    }
    return true;
  }

  void printCaps() {
    v4l2_capability cap;
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0)
      throw invalid_argument(string("invalid video device"));

    printf("Driver: %s\n", (char *)cap.driver);
    printf("card:   %s\n", (char *)cap.card);
    printf("ver:    %08x\n", cap.version);
    printf("caps:   %08x\n", cap.capabilities);
    printf("devcaps:%08x\n", cap.device_caps);
    if (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) {
      printf("        Video Capture\n");
    }
    if (cap.device_caps & V4L2_CAP_STREAMING) {
      printf("        Streaming\n");
    }
  }
  void printStandards() {
    v4l2_input input;
    v4l2_standard standard;

    memset(&input, 0, sizeof(input));

    if (ioctl(m_fd, VIDIOC_G_INPUT, &input.index) < 0)
      throw invalid_argument(string("VIDIOC_G_INPUT"));

    if (ioctl(m_fd, VIDIOC_ENUMINPUT, &input) < 0)
      throw invalid_argument(string("VIDIOC_ENUMINPUT"));

    printf("Current input %s supports:\n", input.name);

    memset(&standard, 0, sizeof(standard));
    standard.index = 0;

    while (ioctl(m_fd, VIDIOC_ENUMSTD, &standard) == 0) {
      if (standard.id & input.std)
        printf("%s\n", standard.name);
      standard.index++;
    }
  }
  void enumerateMenu(v4l2_query_ext_ctrl &ctrl) {
    v4l2_querymenu menu;

    printf("  Menu items:\n");
    memset(&menu, 0, sizeof(menu));
    menu.id = ctrl.id;

    for (menu.index = ctrl.minimum; menu.index < ctrl.maximum; menu.index++) {
      if (!ioctl(m_fd, VIDIOC_QUERYMENU, &menu))
        printf("    %-3d %s\n", menu.index, menu.name);
    }
  }
  void printControls() {
    v4l2_query_ext_ctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;

    while (!ioctl(m_fd, VIDIOC_QUERY_EXT_CTRL, &queryctrl)) {
      if (!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
        printf("Control %08x - %s  Min: %lld  Max: %lld\n", queryctrl.id,
               queryctrl.name, queryctrl.minimum, queryctrl.maximum);

        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
          enumerateMenu(queryctrl);
      }
      queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;
    }
  }
  inline int nBuffers() const { return m_nBuffers; }
};
#endif
