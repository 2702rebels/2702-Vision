#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include <string>
using namespace std;
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>

#include <LoggingUtils.h>
#include <VisionLog.h>
#include <cscore/cscore_cv.h>
#include <cameraserver/cameraserver/CameraServer.h>
#include <drm/drm_fourcc.h>
#include <linux/videodev2.h>

extern "C" {
#include "genio.h"
};

#include "v4lcam.h"
#include "fbdisp.h"
#include "nvcam.h"

namespace fs = std::filesystem;
using namespace spdlog;
using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;
static constexpr auto MC_WAITTIME = std::chrono::milliseconds(750);
static constexpr int CAMERA_INDEX = 0;
static constexpr int PAN_STEP_SIZE = 2;
static constexpr int TILT_STEP_SIZE = 2;
int imageIndex = 0;
fs::path outdir;
cs::CvSource camServer;
std::mutex mtxCamServer;
bool bFirstFrame = true;

CSerialPort motorControllerPort;

void Pan(int deg)
{
    std::string panMsg = "p" + std::to_string(deg) + "\n";
    motorControllerPort.writeData(panMsg.data(), panMsg.size());
    motorControllerPort.flushBuffers();
    std::this_thread::sleep_for(MC_WAITTIME);
}

void Tilt(int deg)
{
    std::string tiltMsg = "t" + std::to_string(deg) + "\n";
    motorControllerPort.writeData(tiltMsg.data(), tiltMsg.size());
    motorControllerPort.flushBuffers();
    std::this_thread::sleep_for(MC_WAITTIME);
}

imgProc proc{};

class imgProc : public nvFrmProcessor
{
    std::mutex	m_mtxCS;
    bool		m_bFirstVid = true;
    cs::CvSource 	m_outputStream;
    cv::Mat m_lastFrame;
    std::mutex m_lastFrameMtx;

    volatile bool	m_bCapture = false;
    int				m_nCapture = 0;

public:
    imgProc()
    {
    }
    virtual ~imgProc()
    {

    }
    virtual void ProcessFrame( nvCamera *pCam,  const nvFrame *pcf )
    {
        if (m_mtxCS.try_lock()) {
            cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC1, (void *)pcf->pu8Image() );
            if (m_bFirstVid) {
                m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows() );
                m_bFirstVid = false;
            }
            m_outputStream.PutFrame(frame);

            if (m_bCapture) {
                m_bCapture = false;
                char szTmp[128];
                sprintf( szTmp, g_pszImagPat, ++m_nCapture );
                cv::imwrite( szTmp, frame );
            }
            m_mtxCS.unlock();
        }
        if (m_lastFrameMtx.try_lock())
        {
            m_lastFrame = cv::Mat(pcf->nRows(), pcf->nCols(), CV_8UC1, (void*)pcf->pu8Image());
            m_lastFrameMtx.unlock();
        }
    }
    void vCaptureFrame()
    {
        m_bCapture = true;
    }
    int nCaptures() const
    {
        return m_nCapture;
    }

    void GetLastFrame(cv::Mat& dest)
    {
        std::unique_lock<std::mutex> lastFrameLock(m_lastFrameMtx);
        dest = m_lastFrame.clone();
    }
};

cv::Size checkerboardPatternSize;

cv::Mat CaptureGrayscaleImage()
{
    cv::Mat image;
    proc.GetLastFrame(image);
    if (image.channels() > 1)
    {
        cv::Mat grayscale;
        cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
        if (bFirstFrame)
        {
            camServer = frc::CameraServer::PutVideo("Vision", grayscale.cols, grayscale.rows);
            bFirstFrame = false;
        }
        if (mtxCamServer.try_lock())
        {
            camServer.PutFrame(grayscale);
        }
        return grayscale;
    }
    if (bFirstFrame)
    {
        camServer = frc::CameraServer::PutVideo("Vision", image.cols, image.rows);
        bFirstFrame = false;
    }
    if (mtxCamServer.try_lock())
    {
        camServer.PutFrame(image);
    }
    return image;
}

void vSetIntegration( nvCamera &nvc )
{
    char szTmp[128];

    GetNumber( szTmp, sizeof(szTmp), "Integration", "Please enter the integration Value" );
    unsigned u = atoi(szTmp);
    nvc.bSetExposure( u );
}

void vSetGain( nvCamera &nvc )
{
    char szTmp[128];

    GetNumber( szTmp, sizeof(szTmp), "Gain", "Please enter the gain Value" );
    unsigned u = atoi(szTmp);
    nvc.bSetGain( u );
}


cv::Size GetCaptureSize()
{
    cv::Mat img = CaptureGrayscaleImage();
    return cv::Size(img.cols, img.rows);
}

bool TakeCalibrationImage(std::string dist)
{
    bool cornersFound = false;
    cv::Mat grayscale = CaptureGrayscaleImage();
    cv::Mat debugImage = grayscale.clone();
    std::vector<cv::Point2f> corners;
    cornersFound = findChessboardCorners(grayscale, checkerboardPatternSize, corners,
                                         cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE);
    if (!cornersFound)
    {
        std::cout << "No checkerboard found!" << std::endl;
    } else
    {
        drawChessboardCorners(debugImage, checkerboardPatternSize, corners, cornersFound);
    }
    cv::imwrite((outdir / dist / ("img-" + std::to_string(imageIndex) + "-" + dist + ".png")).string(), grayscale);
    imageIndex++;
    return cornersFound;
}
int GetChessboardHeightDeg()
{

    Pan(90);
    Tilt(90);
    std::vector<cv::Point2f> corners;
    bool foundChessboard = false;
    while (!foundChessboard)
    {
        cv::Mat grayscale = CaptureGrayscaleImage();
        foundChessboard = cv::findChessboardCorners(grayscale, checkerboardPatternSize, corners);
    }
    cv::Point2f minPt(1e9, 1e9);
    cv::Point2f maxPt(-1e9, -1e9);

    for (const auto& p : corners) {
        minPt.x = std::min(minPt.x, p.x);
        minPt.y = std::min(minPt.y, p.y);
        maxPt.x = std::max(maxPt.x, p.x);
        maxPt.y = std::max(maxPt.y, p.y);
    }

    cv::Rect chessRect(minPt, maxPt);
    int i = 90;
    while (true)
    {
        i += TILT_STEP_SIZE;
        Tilt(i);
        cv::Mat grayscale = CaptureGrayscaleImage();
        bool found = cv::findChessboardCorners(grayscale, checkerboardPatternSize, corners);
        if (found)
        {
            cv::Point2f minChessPt(1e9, 1e9);
            cv::Point2f maxChessPt(-1e9, -1e9);

            for (const auto& p : corners) {
                minChessPt.x = std::min(minChessPt.x, p.x);
                minChessPt.y = std::min(minChessPt.y, p.y);
                maxChessPt.x = std::max(maxChessPt.x, p.x);
                maxChessPt.y = std::max(maxChessPt.y, p.y);
            }
            cv::Rect curChess(minChessPt, maxChessPt);
            cv::Rect intersect = chessRect & curChess;
            if (intersect.area() <= 1e-6)
            {
                break;
            }
        }
    }
    return i - 90;
}

int GetChessboardWidthDeg()
{
    Pan(90);
    Tilt(90);
    std::vector<cv::Point2f> corners;
    bool foundChessboard = false;
    while (!foundChessboard)
    {
        cv::Mat grayscale = CaptureGrayscaleImage();
        foundChessboard = cv::findChessboardCorners(grayscale, checkerboardPatternSize, corners);
    }
    cv::Point2f minPt(1e9, 1e9);
    cv::Point2f maxPt(-1e9, -1e9);

    for (const auto& p : corners) {
        minPt.x = std::min(minPt.x, p.x);
        minPt.y = std::min(minPt.y, p.y);
        maxPt.x = std::max(maxPt.x, p.x);
        maxPt.y = std::max(maxPt.y, p.y);
    }

    cv::Rect chessRect(minPt, maxPt);
    int i = 90;
    while (true)
    {
        i += PAN_STEP_SIZE;
        Pan(i);
        cv::Mat grayscale = CaptureGrayscaleImage();
        bool found = cv::findChessboardCorners(grayscale, checkerboardPatternSize, corners);
        if (found)
        {
            cv::Point2f minChessPt(1e9, 1e9);
            cv::Point2f maxChessPt(-1e9, -1e9);

            for (const auto& p : corners) {
                minChessPt.x = std::min(minChessPt.x, p.x);
                minChessPt.y = std::min(minChessPt.y, p.y);
                maxChessPt.x = std::max(maxChessPt.x, p.x);
                maxChessPt.y = std::max(maxChessPt.y, p.y);
            }
            cv::Rect curChess(minChessPt, maxChessPt);
            cv::Rect intersect = chessRect & curChess;
            if (intersect.area() <= 1e-6)
            {
                break;
            }
        }
    }
    return i - 90;
}

std::pair<int, int> GetTiltFOV()
{
    Pan(90);
    Tilt(90);
    int minTilt = -1;
    for (int i = 90; i > 0; i -= TILT_STEP_SIZE)
    {
        Tilt(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (!findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            minTilt = i + TILT_STEP_SIZE;
            break;
        }
    }
    if (minTilt == -1)
    {
        throw std::runtime_error("Couldn't find minimum tilt to see checkerboard");
    }
    Tilt(90);
    int maxTilt = 181;
    for (int i = 90; i < 180; i += TILT_STEP_SIZE)
    {
        Tilt(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (!findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            maxTilt = i - TILT_STEP_SIZE;
            break;
        }
    }
    if (maxTilt == 181)
    {
        throw std::runtime_error("Couldn't find maximum tilt to see checkerboard");
    }
    if (maxTilt < minTilt)
    {
        int buf = minTilt;
        minTilt = maxTilt;
        maxTilt = buf;
    }
    return {minTilt, maxTilt};
}

std::pair<int, int> GetPanFOV()
{
    Pan(90);
    Tilt(90);
    int minPan = -1;
    for (int i = 90; i > 0; i -= PAN_STEP_SIZE)
    {
        Pan(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (!findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            minPan = i + PAN_STEP_SIZE;
            break;
        }
    }
    if (minPan == -1)
    {
        throw std::runtime_error("Couldn't find minimum tilt to see checkerboard");
    }
    Pan(90);
    int maxPan = 181;
    for (int i = 90; i < 180; i += PAN_STEP_SIZE)
    {
        Pan(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (!findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            maxPan = i - PAN_STEP_SIZE;
            break;
        }
    }
    if (maxPan == 181)
    {
        throw std::runtime_error("Couldn't find maximum tilt to see checkerboard");
    }
    if (maxPan < minPan)
    {
        int buf = minPan;
        minPan = maxPan;
        maxPan = buf;
    }
    return {minPan, maxPan};
}

int main(int argc, char** argv)
{
    SetupTextLogger();
    nvCamera mycam0( pszVideoDev, 0, 1280, 720, V4L2_PIX_FMT_GREY, proc );
    std::string portname;
    std::cout << "Enter port name(eg COM3): ";
    std::cin >> portname;
    std::cout << "Enter output directory of png files: ";
    std::cin >> outdir;
    int checkerBoardWidth;
    int checkerBoardHeight;
    std::cout << "Enter width of checkerboard in # of squares: ";
    std::cin >> checkerBoardWidth;
    std::cout << "Enter height of checkerboard in # of squares: ";
    std::cin >> checkerBoardHeight;
    checkerboardPatternSize = cv::Size(checkerBoardWidth - 1, checkerBoardHeight - 1);
    if (!fs::exists(outdir) || !fs::is_directory(outdir))
    {
        fs::create_directories(outdir);
        fs::create_directories(outdir / "filtered");
    }
    std::cout << "Using serial port " << portname << std::endl;
    motorControllerPort.init(
        portname.data(),
        MC_BAUDRATE,
        ParityNone,
        DataBits8,
        StopOne,
        FlowNone
    );
    if (!motorControllerPort.open())
    {
        std::cerr << "Failed to open serial connection to motor controller" << std::endl;
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    while (true)
    {
        Tilt(90);
        Pan(90);
        std::string dist;
        std::cout << "Enter distance(enter 0 to end / quit): ";
        std::cin >> dist;
        if (dist == "0")
        {
            break;
        }
        fs::create_directories(outdir / dist);
        fs::create_directories(outdir / "filtered" / dist);
        std::cout << "Waiting for chessboard..." << std::endl;
        bool foundChessboard = false;
        while (!foundChessboard)
        {
            cv::Mat img = CaptureGrayscaleImage();
            std::vector<cv::Point2f> corners;
            foundChessboard = findChessboardCorners(img, checkerboardPatternSize, corners,
                                      cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE);
        }
        std::cout << "Found chessboard, starting chessboard size detection & FOV calibration" << std::endl;
        int checkerboardWidthDeg = GetChessboardWidthDeg();
        int checkerboardHeightDeg = GetChessboardHeightDeg();
        info("chessboard width(deg): {}, chessboard height(deg): {}", checkerboardWidthDeg, checkerboardHeightDeg);
        std::pair<int, int> tiltFov = GetTiltFOV();
        info("Tilt FOV min: {}, Tilt FOV max: {}", tiltFov.first, tiltFov.second);
        std::pair<int, int> panFov = GetPanFOV();
        info("Pan FOV min: {}, Pan FOV max: {}", panFov.first, panFov.second);
        int totalSteps = ((panFov.second - panFov.first) / checkerboardWidthDeg) * ( (tiltFov.second - tiltFov.first) / checkerboardHeightDeg);
        info("Total # of steps: {}", totalSteps);
        int stepsDone = 0;
        for (int t = tiltFov.first; t < tiltFov.second; t += checkerboardHeightDeg)
        {
            for (int p = panFov.first; p < panFov.second; p += checkerboardWidthDeg)
            {
                info("Step {}, {}% done", stepsDone, static_cast<double>(stepsDone) / static_cast<double>(totalSteps) * 100);
                Tilt(t);
                Pan(p);
                if (!TakeCalibrationImage(dist))
                {
                    warn("Failed to find checkerboard at p{} t{} which was inside of the pan and tilt fov", p, t);
                }
                stepsDone++;
            }
        }
    }
    motorControllerPort.close();
    return 0;
}