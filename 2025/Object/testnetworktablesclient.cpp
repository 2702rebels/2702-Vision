#include <iostream>
#include <networktables/NetworkTable.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/IntegerTopic.h>
#include <networktables/DoubleTopic.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <future>
#include <map>

using namespace std::literals;


bool bStop = false;
bool readyForNewCmdThread = false;
std::map<std::string, std::function<void()>> commandMap;

nt::IntegerSubscriber hueMinTopic;
nt::IntegerSubscriber hueMaxTopic;
nt::IntegerSubscriber saturationMinTopic;
nt::IntegerSubscriber saturationMaxTopic;
nt::IntegerSubscriber valueMinTopic;
nt::IntegerSubscriber valueMaxTopic;
nt::DoublePublisher coralDistanceTopic;
nt::DoublePublisher coralAngleTopic;

void sighandler(int nSig){
    printf("Received Signal: %d\n", nSig);
    std::cout << "Can't use Ctrl+C to exit, use stop command\n";
}

void showHelp(){
    std::cout << "Valid commands are: \n" << "print - logs all current topic values\n" << "pushdist - push a new distance value\n"
    << "pushangle - push a new angle value\n" << "stop - shuts the server down\n" << "help - displays this text\n";
}

void shutdown(){
    bStop = true;
}

void printNetworkTableValues(){
    printf("huemin: %ld\n", hueMinTopic.GetAtomic(0.0f).value);
    printf("huemax: %ld\n", hueMaxTopic.GetAtomic(0.0f).value);
    printf("satmin: %ld\n", saturationMinTopic.GetAtomic(0.0f).value);
    printf("satmax: %ld\n", saturationMaxTopic.GetAtomic(0.0f).value);
    printf("valmin: %ld\n", valueMinTopic.GetAtomic(0.0f).value);
    printf("valmax: %ld\n", valueMaxTopic.GetAtomic(0.0f).value);
}

void pushDistanceValue(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    coralDistanceTopic.SetDefault(stof(value));
}

void pushAngleValue(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    coralAngleTopic.SetDefault(stof(value));
}

int main( int argc, char **argv ){
    signal(SIGINT, sighandler); // Handles Ctrl+C interrupts
    commandMap["help"] = showHelp;
    commandMap["stop"] = shutdown;
    commandMap["print"] = printNetworkTableValues;
    commandMap["pushdist"] = pushDistanceValue;
    commandMap["pushangle"] = pushAngleValue;
    nt::NetworkTableInstance netTableManager = nt::NetworkTableInstance::GetDefault();
    netTableManager.StartClient4("testnettableclient");
    netTableManager.SetServerTeam(2702);
    auto objectDetectionTable = netTableManager.GetTable("ObjectDetection");
    hueMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/HueMin").Subscribe(0);
    hueMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/HueMax").Subscribe(0);
    saturationMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/SaturationMin").Subscribe(0);
    saturationMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/SaturationMax").Subscribe(0);
    valueMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/ValueMin").Subscribe(0);
    valueMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/ValueMax").Subscribe(0);
    coralDistanceTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/CoralDistance").Publish();
    coralAngleTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/CoralAngle").Publish();
    showHelp();
    auto commandInputThread = std::async(std::launch::async, [] {
        std::string input = ""s;
        if(std::cin >> input) return input;
    });
    while(!bStop){
        std::cout << "NetworkTables are " << (netTableManager.IsConnected() ? "Connected" : "Not Connected") << "\n";
        if(readyForNewCmdThread){
            commandInputThread = std::async(std::launch::async, [] {
                std::string input = ""s;
                if(std::cin >> input) return input;
            });
            readyForNewCmdThread = false;
        }
        if(commandInputThread.wait_for(0.5s) == std::future_status::ready){
            std::string commandInput = commandInputThread.get();
            if(commandMap[commandInput] != NULL){
                commandMap[commandInput]();
            }
            else{
                std::cout << "Command not recognized\n";
            }
            readyForNewCmdThread = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    netTableManager.StopClient();
    return 0;
}