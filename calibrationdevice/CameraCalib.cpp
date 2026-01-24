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
// todo: produce the output coefficients
// todo: show heatmap of where most of the checkerboard squares were
namespace fs = std::filesystem;
using namespace spdlog;
using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;
static constexpr auto MC_WAITTIME = std::chrono::milliseconds(750);
static constexpr int ANGLE_INTERVAL = 5;
static constexpr int CAMERA_INDEX = 0;
static cv::Size checkerboardPatternSize = cv::Size(9, 6);
std::vector<std::vector<cv::Point3f>> objectPoints;
std::vector<std::vector<cv::Point2f>> imagePoints;
std::vector<cv::Point3f> objp;
int imageIndex = 0;
fs::path outdir;

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

cv::Size GetCaptureSize(cv::VideoCapture& camCapture)
{
    cv::Mat img;
    camCapture >> img;
    return cv::Size(img.cols, img.rows);
}

void TiltAndImage(int deg, cv::VideoCapture& camCapture)
{
    Tilt(deg);
    bool cornersFound = false;
    cv::Mat image, grayscale;
    do
    {
        camCapture >> image;
        if (image.empty())
        {
            std::cerr << "Failed to capture image" << std::endl;
            throw;
        }
        if (image.channels() > 1)
        {
            cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
        }
        else
        {
            grayscale = image.clone();
        }
        std::vector<cv::Point2f> corners;
        cornersFound = findChessboardCorners(grayscale, checkerboardPatternSize, corners,
            cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE);
        if (!cornersFound)
        {
            cv::putText(image, "No checkerboard found!", cv::Point(30,30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0));
        } else
        {
            cornerSubPix(grayscale, corners, cv::Size(11, 11), cv::Size(-1, -1),
            cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.0001));
            objectPoints.push_back(objp);
            imagePoints.push_back(corners);

            // Draw and display the corners (optional)
            drawChessboardCorners(image, checkerboardPatternSize, corners, cornersFound);
        }
        //cv::imshow("Captured Image (Filtered)" , image);
        //cv::waitKey(1);
    } while (cornersFound == false);
    cv::imwrite((outdir / ("img-" + std::to_string(imageIndex) + ".png")).string(), grayscale);
    cv::imwrite((outdir / ("img-filtered-" + std::to_string(imageIndex) + ".png")).string(), image);
    imageIndex++;
}

int main(int argc, char** argv) {
    SetupTextLogger();
    for (int i = 0; i < checkerboardPatternSize.height; ++i) {
        for (int j = 0; j < checkerboardPatternSize.width; ++j) {
            objp.push_back(cv::Point3f(j, i, 0));
        }
    }
    cv::VideoCapture camCapture(CAMERA_INDEX);
    if (!camCapture.isOpened())
    {
        std::cerr << "Failed to open camera" << std::endl;
        return 1;
    }
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
    for (int i = 78; i < 135; i += ANGLE_INTERVAL)
    {
        TiltAndImage(i, camCapture);
    }
    for (int i = 0; i < 45; i += ANGLE_INTERVAL)
    {
        TiltAndImage(i, camCapture);
    }
    motorControllerPort.close();
    cv::destroyAllWindows();
    std::cout << "image-taking job complete, transforming into calibration coefficients..." << std::endl;
    cv::Mat cameraMatrix, distCoeffs, rvecs, tvecs;
    cv::calibrateCamera(objectPoints, imagePoints, GetCaptureSize(camCapture), cameraMatrix, distCoeffs, rvecs, tvecs);
    info("Camera Matrix:\n {}, {}, {},\n{}, {}, {},\n{}, {}, {}",
    cameraMatrix.at<float>(0, 0), cameraMatrix.at<float>(0, 1), cameraMatrix.at<float>(0, 2),
    cameraMatrix.at<float>(1, 0), cameraMatrix.at<float>(1, 1), cameraMatrix.at<float>(1, 2),
    cameraMatrix.at<float>(2, 0), cameraMatrix.at<float>(2, 1), cameraMatrix.at<float>(2, 2)
);
    info("Distortion Coefficients :\n {}, {}, {}, {}, {}",
        distCoeffs.at<float>(0, 0), distCoeffs.at<float>(0, 1), distCoeffs.at<float>(0, 2), distCoeffs.at<float>(0, 3), distCoeffs.at<float>(0, 4)
    );
    return 0;
}