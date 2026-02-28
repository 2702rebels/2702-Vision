#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <string>
#include <thread>

using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;
static constexpr auto MC_WAITTIME = std::chrono::milliseconds(750);

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

int main()
{
    std::string portname;
    std::cout << "Enter port name(eg COM3): ";
    std::cin >> portname;
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
    int tilt;
    int pan;
    std::cout << "enter tilt: ";
    std::cin >> tilt;
    std::cout << "enter pan: ";
    std::cin >> pan;
    Tilt(tilt);
    Pan(pan);
    return 0;
}