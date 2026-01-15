#include <iostream>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>

using namespace itas109;

static constexpr int MC_BAUDRATE = 115200;

std::string GetPortName(int argc, char** argv)
{
    std::string portname;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--port-name" && i + 1 < argc)
        {
            portname = argv[i++];
            return portname;
        }
    }
    // TODO: Autodetect port here, we would have to write 2 versions, one for windows and one for linux though
    return "COM3";
}

int main(int argc, char** argv) {
    std::string portname = GetPortName(argc, argv);
    std::cout << "Using serial port " << portname << std::endl;
    std::string outdir = "./";
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--outdir" && i + 1 < argc)
        {
            outdir = argv[i++];
        }
    }
    std::cout << "hello" << std::endl;
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