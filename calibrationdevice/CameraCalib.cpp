#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include <string>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>

#include <LoggingUtils.h>
#include <frmprocessor.h>
#include <VisionLog.h>
#include <cscore/cscore_cv.h>
#include <cameraserver/cameraserver/CameraServer.h>
#ifndef DESKTOP
#include <rpicam.h>
#include <drm/drm_fourcc.h>
#include <linux/videodev2.h>
#endif
/** TODO LIST:
 * - Detection of FOV (tilt and pan)
 * - Use pan/tilt FOV to xy take images of the checkerboard all over the sensor
 * - Fix error when outputting calibration coefficients
*/
namespace fs = std::filesystem;
using namespace spdlog;
using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;
static constexpr auto MC_WAITTIME = std::chrono::milliseconds(750);
static constexpr int CAMERA_INDEX = 0;
static constexpr int PAN_STEP_SIZE = 5;
static constexpr int TILT_STEP_SIZE = 5;
int imageIndex = 0;
fs::path outdir;
cs::CvSource camServer;
std::mutex mtxCamServer;
bool bFirstFrame = true;

CSerialPort motorControllerPort;

class imgProc : public FrmProcessor
{
    cv::Mat m_matLastFrame;
    std::mutex m_mtxLastFrame;

public:
    virtual ~imgProc()
    {
    }

    virtual void ProcessFrame(int nCam, CameraFrame const* pcf)
    {
        cv::Mat frame(pcf->nRows(), pcf->nCols(), CV_8UC3, (void*)pcf->pu8Image());
        if (m_mtxLastFrame.try_lock())
        {
            m_matLastFrame = frame;
            m_mtxLastFrame.unlock();
        }
    }

    void GetLastFrame(cv::Mat& frame)
    {
        std::unique_lock<std::mutex> mtx(m_mtxLastFrame);
        frame = m_matLastFrame.clone();
    }
};

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

cv::Size checkerboardPatternSize;

imgProc proc{};

cv::Mat CaptureGrayscaleImage()
{
    cv::Mat image;
#if DESKTOP
    cv::VideoCapture camCapture(CAMERA_INDEX);
    if (!camCapture.isOpened())
    {
        std::cerr << "Failed to open camera" << std::endl;
        throw std::runtime_error("Failed to open camera");
    }
    camCapture >> image;
    if (image.empty())
    {
        throw std::runtime_error("Failed to capture image from camera");
    }
#else
    proc.GetLastFrame(image);
#endif
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

cv::Size GetCaptureSize()
{
    cv::Mat img = CaptureGrayscaleImage();
    return cv::Size(img.cols, img.rows);
}

bool TakeCalibrationImage(int dist)
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
        return false;
    }

    drawChessboardCorners(debugImage, checkerboardPatternSize, corners, cornersFound);
#if DESKTOP
    cv::imshow("Captured Image (Filtered)", debugImage);
    cv::waitKey(1);
#endif
    cv::imwrite((outdir / std::to_string(dist) / ("img-" + std::to_string(imageIndex) + ".png")).string(), grayscale);
    cv::imwrite((outdir / "filtered" / std::to_string(dist) / ("img-" + std::to_string(imageIndex) + ".png")).string(), debugImage);
    imageIndex++;
    return true;
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
#ifndef DESKTOP
    RPICamera mycam0("/dev/video0", 0, 640, 480, DRM_FORMAT_RGB888, proc);
#endif
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
    Tilt(78);
    Pan(90);
    while (true)
    {
        int dist;
        std::cout << "Enter distance(enter 0 to end / quit): ";
        std::cin >> dist;
        if (dist == 0)
        {
            break;
        }
        fs::create_directories(outdir / std::to_string(dist));
        std::cout << "Waiting for chessboard..." << std::endl;
        bool foundChessboard = false;
        while (!foundChessboard)
        {
            cv::Mat img = CaptureGrayscaleImage();
            std::vector<cv::Point2f> corners;
            foundChessboard = findChessboardCorners(img, checkerboardPatternSize, corners,
                                      cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE);
        }
        std::cout << "Found chessboard, starting FOV calibration" << std::endl;
        std::pair<int, int> tiltFov = GetTiltFOV();
        info("Tilt FOV min: {}, Tilt FOV max: {}", tiltFov.first, tiltFov.second);
        std::pair<int, int> panFov = GetPanFOV();
        info("Pan FOV min: {}, Pan FOV max: {}", panFov.first, panFov.second);
        int totalSteps = ((panFov.second - panFov.first) / PAN_STEP_SIZE) * ( (tiltFov.second - tiltFov.first) / TILT_STEP_SIZE);
        info("Total # of steps: {}", totalSteps);
        int stepsDone = 0;
        for (int t = tiltFov.first; t < tiltFov.second; t += TILT_STEP_SIZE)
        {
            for (int p = panFov.first; p < panFov.second; p += PAN_STEP_SIZE)
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
#if DESKTOP
    cv::destroyAllWindows();
#endif
    return 0;
}
