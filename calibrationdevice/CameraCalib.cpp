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
static constexpr int PAN_STEP_SIZE = 2;
static constexpr int TILT_STEP_SIZE = 2;
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
        } else
        {
            break;
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
        } else
        {
            break;
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
    std::string cameraPath;
    std::cout << "Enter camera path(eg /dev/video0): ";
    std::cin >> cameraPath;
#ifndef DESKTOP
    RPICamera mycam0(cameraPath.c_str(), 0, 640, 480, DRM_FORMAT_RGB888, proc);
#endif
    std::string portname;
    std::cout << "Enter USB port name(eg COM3 or /dev/ttyUSB0): ";
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
        info("Detecting chessboard width...");
        int checkerboardWidthDeg = GetChessboardWidthDeg();
        info("Detecting chessboard height...");
        int checkerboardHeightDeg = GetChessboardHeightDeg();
        info("chessboard width(deg): {}, chessboard height(deg): {}", checkerboardWidthDeg, checkerboardHeightDeg);
        info("Finding tilt fov...");
        std::pair<int, int> tiltFov = GetTiltFOV();
        info("Tilt FOV min: {}, Tilt FOV max: {}", tiltFov.first, tiltFov.second);
        info("Finding pan fov...");
        std::pair<int, int> panFov = GetPanFOV();
        info("Pan FOV min: {}, Pan FOV max: {}", panFov.first, panFov.second);
        int panFovWidth = panFov.second - panFov.first;
        int tiltFovHeight = tiltFov.second - tiltFov.first;
        int nChessboardsWide = floor(panFovWidth / checkerboardWidthDeg);
        int nChessboardsTall = floor(tiltFovHeight / checkerboardHeightDeg);
        int widthUnusedSpace = panFovWidth - nChessboardsWide * checkerboardWidthDeg;
        int heightUnusedSpace = tiltFovHeight - nChessboardsTall * checkerboardHeightDeg;
        std::vector<cv::Point2i> boardPositions;
		int curLeftEdge = panFov.first;
		int curTopEdge = tiltFov.first;
        if(static_cast<int>(widthUnusedSpace / nChessboardsWide) < round(widthUnusedSpace / nChessboardsWide)){
             curLeftEdge += floor(widthUnusedSpace / nChessboardsWide);
		} else {
			curLeftEdge += ceil(widthUnusedSpace / nChessboardsWide);
		}
		if(static_cast<int>(heightUnusedSpace / nChessboardsTall) < round(heightUnusedSpace / nChessboardsTall)){
			curTopEdge += floor(heightUnusedSpace / nChessboardsTall);
		} else {
			curTopEdge += ceil(heightUnusedSpace / nChessboardsTall);
		}
        int initTopEdge = curTopEdge;
        int initLeftEdge = curLeftEdge;
		for(int i = 0; i < nChessboardsWide; i++){
            for(int j = 0; j < nChessboardsTall; j++){
				boardPositions.emplace_back(round(curLeftEdge + 0.5 * checkerboardWidthDeg), round(curTopEdge + 0.5 * checkerboardHeightDeg));
                curTopEdge += static_cast<int>(std::round(checkerboardHeightDeg + heightUnusedSpace / nChessboardsTall));
            }
            curTopEdge = initTopEdge;
            curLeftEdge += static_cast<int>(std::round((checkerboardWidthDeg + widthUnusedSpace / nChessboardsWide));
		}
        int stepsDone = 0;
        curLeftEdge = initLeftEdge + 1;
        for (int i = 0; i < ceil(panFovWidth / checkerboardWidthDeg) + 1; i++)
        {
            boardPositions.emplace_back(curLeftEdge + 0.5 * checkerboardWidthDeg, tiltFov.first + 1);
            curLeftEdge += static_cast<int>(std::round((checkerboardWidthDeg + widthUnusedSpace / nChessboardsWide));
        }
        curLeftEdge = initLeftEdge + 1;
        for (int i = 0; i < ceil(panFovWidth / checkerboardWidthDeg) + 1; i++)
        {
            boardPositions.emplace_back(curLeftEdge + 0.5 * checkerboardWidthDeg, tiltFov.second - 1);
            curLeftEdge += static_cast<int>(std::round((checkerboardWidthDeg + widthUnusedSpace / nChessboardsWide));
        }
        curTopEdge = initTopEdge + 1;
        for (int i = 0; i < ceil(tiltFovHeight / checkerboardHeightDeg) + 1; i++)
        {
            boardPositions.emplace_back(panFov.first - 2, round(curTopEdge + 0.5 * checkerboardHeightDeg));
            curTopEdge += static_cast<int>(std::round(checkerboardHeightDeg + heightUnusedSpace / nChessboardsTall));
        }
        curTopEdge = initTopEdge + 1;
        for (int i = 0; i < ceil(tiltFovHeight / checkerboardHeightDeg) + 1; i++)
        {
            boardPositions.emplace_back(panFov.second - 1, round(curTopEdge + 0.5 * checkerboardHeightDeg));
            curTopEdge += static_cast<int>(std::round((checkerboardHeightDeg + heightUnusedSpace / nChessboardsTall));
        }
        for (cv::Point2i& position : boardPositions)
        {
            info("Step {} out of {} steps, {}% done", stepsDone, boardPositions.size(), round(static_cast<double>(stepsDone) / static_cast<double>(boardPositions.size()) * 100));
            Pan(position.x);
            Tilt(position.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            if(!TakeCalibrationImage(dist)){
				warn("Calibration image not found for board position p{} t{}", position.x, position.y);
			}
            stepsDone++;
        }
    }
    motorControllerPort.close();
    return 0;
}
