/*
 * \brief   This is a program that acts as a network table server to receive and send object detection data
 * \brief   for testing purposes. 
 * \brief   Depends on: 
 * \brief   - wpilib's ntcore
*/

// TODO, some of this should be converted to a class so it can be better documented

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

/* \brief This is where all the commands are stored, in order to add a command, set a key(string) value to a void function like this:
 void testcommand(){
    do your thing here
 }
 commandMap["yourtestcommand"] = testcommand;
*/
std::map<std::string, std::function<void()>> commandMap;

nt::IntegerPublisher hueMinTopic;
nt::IntegerPublisher hueMaxTopic;
nt::IntegerPublisher saturationMinTopic;
nt::IntegerPublisher saturationMaxTopic;
nt::IntegerPublisher valueMinTopic;
nt::IntegerPublisher valueMaxTopic;
nt::DoubleSubscriber coralDistanceTopic;
nt::DoubleSubscriber coralAngleTopic;

/*This is called when Ctrl+C is pressed*/
void sighandler(int nSig){
    printf("Received Signal: %d\n", nSig);
    std::cout << "Can't use Ctrl+C to exit, use stop command\n";
}

/*Prints the help text to console*/
void showHelp(){
    std::cout << "Valid commands are: \n" << "print - logs all current topic values\n" << "pushhuemin - pushes a new value for the huemin param\n"
    << "pushhuemax - pushes a new value for the huemax param\n" << "pushsaturationmin - pushes a new value for the saturationmin param\n" << "pushsaturationmax - pushes a new value for the saturationmax param\n"
    << "pushvaluemin - pushes a new value for the valuemin param\n" << "pushvaluemax - pushes a new value for the valuemax param\n"
    << "stop - shuts the server down\n" << "help - displays this text\n";
}

/* \brief Shuts down the server. Note that this is not immediate, and the program will walk through one more tick before exiting.*/
void shutdown(){
    bStop = true;
}

/* \brief Prints the network table values to console. This does not auto-update as more topics are added, so this should be updated when a new topic is added */
void printNetworkTableValues(){
    printf("Last received coral distance: %.6f\n",coralDistanceTopic.GetAtomic(0.0f).value);
    printf("Last received coral angle: %.6f\n",coralAngleTopic.GetAtomic(0.0f).value);
}

/* \brief Pushes a new hue min value to the associated networktable topic. Value is provided through cin input */
void pushHueMin(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    hueMinTopic.SetDefault(stoi(value));
}

/* \brief Pushes a new hue max value to the associated networktable topic. Value is provided through cin input */
void pushHueMax(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    hueMaxTopic.SetDefault(stoi(value));
}

/* \brief Pushes a new saturation min value to the associated networktable topic. Value is provided through cin input */
void pushSaturationMin(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    saturationMinTopic.SetDefault(stoi(value));
}

/* \brief Pushes a new saturation max value to the associated networktable topic. Value is provided through cin input */
void pushSaturationMax(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    saturationMaxTopic.SetDefault(stoi(value));
}

/* \brief Pushes a new value min value to the associated networktable topic. Value is provided through cin input */
void pushValueMin(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    valueMinTopic.SetDefault(stoi(value));
}

/* \brief Pushes a new value max value to the associated networktable topic. Value is provided through cin input */
void pushValueMax(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    valueMaxTopic.SetDefault(stoi(value));
}

int main( int argc, char **argv ){
    signal(SIGINT, sighandler); // Handles Ctrl+C interrupts
    // Assign commands
    commandMap["help"] = showHelp;
    commandMap["stop"] = shutdown;
    commandMap["print"] = printNetworkTableValues;
    commandMap["pushhuemin"] = pushHueMin;
    commandMap["pushhuemax"] = pushHueMax;
    commandMap["pushsaturationmin"] = pushSaturationMin;
    commandMap["pushsaturationmax"] = pushSaturationMax;
    commandMap["pushvaluemin"] = pushValueMin;
    commandMap["pushvaluemax"] = pushValueMax;
    nt::NetworkTableInstance netTableManager = nt::NetworkTableInstance::GetDefault();
    netTableManager.StartServer(); // Server will accept requests using either the NT3 or NT4 protocols
    auto objectDetectionTable = netTableManager.GetTable("ObjectDetection");
    // Initialize the topics
    hueMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/HueMin").Publish();
    hueMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/HueMax").Publish();
    saturationMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/SaturationMin").Publish();
    saturationMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/SaturationMax").Publish();
    valueMinTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/ValueMin").Publish();
    valueMaxTopic = objectDetectionTable->GetIntegerTopic("ObjectDetection/ValueMax").Publish();
    coralDistanceTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/CoralDistance").Subscribe(0.0f);
    coralAngleTopic = objectDetectionTable->GetDoubleTopic("ObjectDetection/CoralAngle").Subscribe(0.0f);
    showHelp();
    // Start an async thread to handle user input, this allows the server to keep responding to requests while waiting for input
    auto commandInputThread = std::async(std::launch::async, [] {
        std::string input = ""s;
        if(std::cin >> input) return input;
    });
    while(!bStop){
        if(readyForNewCmdThread){
            commandInputThread = std::async(std::launch::async, [] {
                std::string input = ""s;
                if(std::cin >> input) return input;
            });
            readyForNewCmdThread = false;
        }
        // If the input thread has received its input, handle it
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
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "Exiting...\n";
    netTableManager.StopServer();
    return 0;
}