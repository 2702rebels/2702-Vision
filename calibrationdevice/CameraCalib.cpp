#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>

using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;

int main(int argc, char** argv) {
    std::string portname;
    std::cout << "Enter port name(eg COM3): " << std::endl;
    std::cin >> portname;
    std::cout << "Using serial port " << portname << std::endl;
    CSerialPort motorControllerPort;
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
    std::string msg1 = "t90\n";
    std::string msg2 = "p90\n";
    motorControllerPort.writeData(msg1.data(), msg1.size());
    motorControllerPort.writeData(msg2.data(), msg2.size());
    motorControllerPort.close();
    std::cout << "Calibration complete!" << std::endl;
    return 0;
}