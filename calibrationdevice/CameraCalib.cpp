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
static constexpr int PAN_STEP_SIZE = 1;
static constexpr int TILT_STEP_SIZE = 1;
static cv::Size checkerboardPatternSize = cv::Size(9, 6);
std::vector<std::vector<cv::Point3f>> objectPoints;
std::vector<std::vector<cv::Point2f>> imagePoints;
std::vector<cv::Point3f> objp;
int imageIndex = 0;
fs::path outdir;

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

imgProc proc{};

cv::Mat CaptureGrayscaleImage()
{
#if DESKTOP
    cv::VideoCapture camCapture(CAMERA_INDEX);
    if (!camCapture.isOpened())
    {
        std::cerr << "Failed to open camera" << std::endl;
        throw std::runtime_error("Failed to open camera");
    }
    cv::Mat image;
    camCapture >> image;
    if (image.empty())
    {
        throw std::runtime_error("Failed to capture image from camera");
    }
#else
    image = proc.GetLastFrame();
#endif
    if (image.channels() > 1)
    {
        cv::Mat grayscale;
        cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
        return grayscale;
    }
    return image;
}

cv::Size GetCaptureSize()
{
    cv::Mat img = CaptureGrayscaleImage();
    return cv::Size(img.cols, img.rows);
}

bool TakeCalibrationImage()
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
    cornerSubPix(grayscale, corners, cv::Size(11, 11), cv::Size(-1, -1),
                 cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.0001));
    objectPoints.push_back(objp);
    imagePoints.push_back(corners);

    drawChessboardCorners(debugImage, checkerboardPatternSize, corners, cornersFound);
#if DESKTOP
    cv::imshow("Captured Image (Filtered)", debugImage);
    cv::waitKey(1);
#endif
    cv::imwrite((outdir / ("img-" + std::to_string(imageIndex) + ".png")).string(), grayscale);
    cv::imwrite((outdir / ("img-filtered-" + std::to_string(imageIndex) + ".png")).string(), debugImage);
    imageIndex++;
    return true;
}

std::pair<int, int> GetTiltFOV()
{
    Pan(90);
    Tilt(0);
    int minTilt = -1;
    for (int i = 0; i < 90; i++)
    {
        Tilt(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            minTilt = i;
            break;
        }
    }
    if (minTilt == -1)
    {
        throw std::runtime_error("Couldn't find minimum tilt to see checkerboard");
    }
    Tilt(180);
    int maxTilt = 181;
    for (int i = 180; i > 90; i--)
    {
        Tilt(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            maxTilt = i;
            break;
        }
    }
    if (maxTilt == 181)
    {
        throw std::runtime_error("Couldn't find maximum tilt to see checkerboard");
    }
    return {minTilt, maxTilt};
}

std::pair<int, int> GetPanFOV()
{
    Pan(0);
    Tilt(90);
    int minPan = -1;
    for (int i = 0; i < 90; i++)
    {
        Pan(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            minPan = i;
            break;
        }
    }
    if (minPan == -1)
    {
        throw std::runtime_error("Couldn't find minimum tilt to see checkerboard");
    }
    Pan(180);
    int maxPan = 181;
    for (int i = 180; i > 90; i--)
    {
        Pan(i);
        cv::Mat img = CaptureGrayscaleImage();
        std::vector<cv::Point2f> corners;
        if (findChessboardCorners(img, checkerboardPatternSize, corners,
                                  cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE))
        {
            maxPan = i;
            break;
        }
    }
    if (maxPan == 181)
    {
        throw std::runtime_error("Couldn't find maximum tilt to see checkerboard");
    }
    return {minPan, maxPan};
}

int main(int argc, char** argv)
{
    SetupTextLogger();
    for (int i = 0; i < checkerboardPatternSize.height; ++i)
    {
        for (int j = 0; j < checkerboardPatternSize.width; ++j)
        {
            objp.push_back(cv::Point3f(j, i, 0));
        }
    }
#ifndef DESKTOP
    RPICamera mycam0("/dev/video0", 0, 640, 480, DRM_FORMAT_RGB888, proc);
#endif
    std::string portname;
    std::cout << "Enter port name(eg COM3): ";
    std::cin >> portname;
    std::cout << "Enter output directory of png files: ";
    std::cin >> outdir;
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
    std::pair<int, int> tiltFov = GetTiltFOV();
    info("Tilt FOV min: {}, Tilt FOV max: {}", tiltFov.first, tiltFov.second);
    std::pair<int, int> panFov = GetPanFOV();
    info("Pan FOV min: {}, Pan FOV max: {}", panFov.first, panFov.second);
    int totalSteps = (panFov.second - panFov.first) / PAN_STEP_SIZE + (tiltFov.second - tiltFov.first) / TILT_STEP_SIZE;
    info("Total # of steps: {}", totalSteps);
    int stepsDone = 0;
    for (int t = tiltFov.first; t < tiltFov.second; t += TILT_STEP_SIZE)
    {
        for (int p = panFov.first; p < panFov.second; p += PAN_STEP_SIZE)
        {
            info("Step {}, {}% done", stepsDone, stepsDone / totalSteps * 100);
            Tilt(t);
            Pan(p);
            if (!TakeCalibrationImage())
            {
                warn("Failed to find checkerboard at p{} t{} which was inside of the pan and tilt fov", p, t);
            }
            stepsDone++;
        }
    }
    motorControllerPort.close();
#if DESKTOP
    cv::destroyAllWindows();
#endif
    std::cout << "image-taking job complete, transforming into calibration coefficients..." << std::endl;
    cv::Mat cameraMatrix, distCoeffs, rvecs, tvecs;
#if DESKTOP
    cv::calibrateCamera(objectPoints, imagePoints, GetCaptureSize(), cameraMatrix, distCoeffs, rvecs, tvecs);
#else
    cv::calibrateCamera(objectPoints, imagePoints, cv::Size(640, 480), cameraMatrix, distCoeffs, rvecs, tvecs);
#endif
    info("Camera Matrix:\n {}, {}, {},\n{}, {}, {},\n{}, {}, {}",
         cameraMatrix.at<float>(0, 0), cameraMatrix.at<float>(0, 1), cameraMatrix.at<float>(0, 2),
         cameraMatrix.at<float>(1, 0), cameraMatrix.at<float>(1, 1), cameraMatrix.at<float>(1, 2),
         cameraMatrix.at<float>(2, 0), cameraMatrix.at<float>(2, 1), cameraMatrix.at<float>(2, 2)
    );
    info("Distortion Coefficients :\n {}, {}, {}, {}, {}",
         distCoeffs.at<float>(0, 0), distCoeffs.at<float>(0, 1), distCoeffs.at<float>(0, 2), distCoeffs.at<float>(0, 3),
         distCoeffs.at<float>(0, 4)
    );
    return 0;
}
