#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include <string>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>

namespace fs = std::filesystem;

using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;
static constexpr auto MC_WAITTIME = std::chrono::milliseconds(750);
static constexpr int ANGLE_INTERVAL = 5;
static constexpr int CAMERA_INDEX = 0;
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

void TiltAndImage(int deg, cv::VideoCapture& camCapture)
{
    Tilt(deg);
    cv::Mat image;
    camCapture >> image;
    if (image.empty())
    {
        std::cerr << "Failed to capture image" << std::endl;
        throw;
    }
    cv::Mat grayscale;
    if (image.channels() > 1)
    {
        cv::cvtColor(image, grayscale, cv::COLOR_BGR2GRAY);
    }
    else
    {
        grayscale = image.clone();
    }
    cv::imwrite((outdir / ("img-" + std::to_string(imageIndex) + ".png")).string(), grayscale);
    imageIndex++;
}

int main(int argc, char** argv) {
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
    Tilt(90);
    Pan(90);
    for (int i = 90; i < 135; i += ANGLE_INTERVAL)
    {
        TiltAndImage(i, camCapture);
    }
    for (int i = 0; i < 45; i += ANGLE_INTERVAL)
    {
        TiltAndImage(i, camCapture);
    }
    motorControllerPort.close();
    std::cout << "Calibration images saved to output directory, use calibrate program to transform them into coefficients" << std::endl;
    return 0;
}