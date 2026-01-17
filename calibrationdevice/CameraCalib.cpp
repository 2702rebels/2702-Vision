#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include <string>

using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;
CSerialPort motorControllerPort;

void Pan(int deg)
{
    std::string panMsg = "p" + std::to_string(deg) + "\n";
    motorControllerPort.writeData(panMsg.data(), panMsg.size());
}

void Tilt(int deg)
{
    std::string tiltMsg = "t" + std::to_string(deg) + "\n";
    motorControllerPort.writeData(tiltMsg.data(), tiltMsg.size());
}

int main(int argc, char** argv) {
    std::string portname;
    std::cout << "Enter port name(eg COM3): " << std::endl;
    std::cin >> portname;
    std::cout << "Using serial port " << portname << std::endl;
    motorControllerPort.init(
        portname.data(),
        MC_BAUDRATE,
        ParityNone,
        DataBits8,
        StopOne,
        FlowNone);
    if (!motorControllerPort.open())
    {
        std::cerr << "Failed to open serial connection to motor controller" << std::endl;
        return 1;
    }
    Pan(0);
    Tilt(0);
    int tiltNum;
    std::cout << "enter tilt num: " << std::endl;
    std::cin >> tiltNum;
    int panNum;
    std::cout << "enter pan num: " << std::endl;
    std::cin >> panNum;
    Tilt(tiltNum);
    Pan(panNum);
    motorControllerPort.close();
    std::cout << "Calibration complete!" << std::endl;
    return 0;
}