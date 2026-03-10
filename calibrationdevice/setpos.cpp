#include <CSerialPort/SerialPort.h>
#include <iostream>
#include <string>
#include <thread>

using SerialPort = itas109::CSerialPort;

static constexpr int MC_BAUDRATE = 115200;
static constexpr auto MC_WAITTIME = std::chrono::milliseconds(750);

static SerialPort motorControllerPort;

static void Pan(int deg) {
    std::string panMsg = "p" + std::to_string(deg) + "\n";
    motorControllerPort.writeData(panMsg.data(), panMsg.size());
    motorControllerPort.flushBuffers();
    std::this_thread::sleep_for(MC_WAITTIME);
}

static void Tilt(int deg) {
    std::string tiltMsg = "t" + std::to_string(deg) + "\n";
    motorControllerPort.writeData(tiltMsg.data(), tiltMsg.size());
    motorControllerPort.flushBuffers();
    std::this_thread::sleep_for(MC_WAITTIME);
}

int main() { // NOLINT
    std::string portname;
    std::cout << "Enter port name(eg COM3): ";
    std::cin >> portname;
    motorControllerPort.init(portname.data(), MC_BAUDRATE, itas109::ParityNone, itas109::DataBits8,
                             itas109::StopOne, itas109::FlowNone);
    if (!motorControllerPort.open()) {
        std::cerr << "Failed to open serial connection to motor controller"
                  << '\n';
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
    int tilt = 0;
    int pan = 0;
    std::cout << "enter tilt: ";
    std::cin >> tilt;
    std::cout << "enter pan: ";
    std::cin >> pan;
    Tilt(tilt);
    Pan(pan);
    return 0;
}