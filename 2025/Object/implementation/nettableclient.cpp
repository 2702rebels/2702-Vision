/*! \brief This is a program that acts as a network tables client to test
 * sending values to a server for debugging purposes, can also print out object
 * detection parameters that are received from the server. Depends on:
 * - wpilib ntcore
 */

#include "LoggingUtils.h"

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <networktables/DoubleTopic.h>
#include <networktables/IntegerTopic.h>
#include <networktables/NetworkTable.h>
#include <networktables/NetworkTableInstance.h>
#include <signal.h>
#include <thread>

using namespace std::literals;

std::map<std::string, std::function<void()>> commandMap;
bool bStop = false;
bool readyForNewCmdThread = false;

nt::IntegerSubscriber hueMinTopic;
nt::IntegerSubscriber hueMaxTopic;
nt::IntegerSubscriber saturationMinTopic;
nt::IntegerSubscriber saturationMaxTopic;
nt::IntegerSubscriber valueMinTopic;
nt::IntegerSubscriber valueMaxTopic;
nt::DoublePublisher coralDistanceTopic;
nt::DoublePublisher coralAngleTopic;
nt::IntegerSubscriber minWidthTopic;
nt::IntegerSubscriber maxWidthTopic;
nt::IntegerSubscriber minHeightTopic;
nt::IntegerSubscriber maxHeightTopic;
nt::IntegerSubscriber minAreaTopic;
nt::IntegerSubscriber maxAreaTopic;
nt::DoubleSubscriber minPercentFillTopic;
nt::DoubleSubscriber camOffsetXTopic;
nt::DoubleSubscriber camOffsetYTopic;
nt::DoubleSubscriber camHeightTopic;
nt::DoubleSubscriber camAngleTopic;
nt::IntegerSubscriber filterWindowTopic;

/*! \brief Handle Ctrl+C interrupts */
void sighandler(int nSig) {
    info("Received Signal: {}", nSig);
    info("Can't use Ctrl+C to exit, use stop command");
}

/*! \brief Print the list of commands to the console */
void showHelp() {
    info("Valid commands are: \nprint - logs all current topic values\npushdist "
         "- push a new distance value\npushangle - push a new angle value\nstop "
         "- shuts the server down\nhelp - displays this text");
}

/*! \brief Shuts down the client. Note that this is not an immediate effect,
 * program will run for another tick before exiting. */
void shutdownClient() {
    bStop = true;
}

/*! \brief Prints the current parameter networktable values.
This function does not automatically update as new topics are added and will
need to be modified if new topics are added
*/
void printNetworkTableValues() {
    info("huemin: {}", hueMinTopic.GetAtomic(0.0f).value);
    info("huemax: {}", hueMaxTopic.GetAtomic(0.0f).value);
    info("satmin: {}", saturationMinTopic.GetAtomic(0.0f).value);
    info("satmax: {}", saturationMaxTopic.GetAtomic(0.0f).value);
    info("valmin: {}", valueMinTopic.GetAtomic(0.0f).value);
    info("valmax: {}", valueMaxTopic.GetAtomic(0.0f).value);
    info("minwidth: {}", minWidthTopic.GetAtomic(0).value);
    info("maxwidth: {}", maxWidthTopic.GetAtomic(0).value);
    info("minheight: {}", minHeightTopic.GetAtomic(0).value);
    info("maxheight: {}", maxHeightTopic.GetAtomic(0).value);
    info("minarea: {}", minAreaTopic.GetAtomic(0).value);
    info("maxarea: {}", maxAreaTopic.GetAtomic(0).value);
    info("minpercentfill: {}", minPercentFillTopic.GetAtomic(0.0f).value);
    info("offsetx: {}", camOffsetXTopic.GetAtomic(0.0f).value);
    info("offsety: {}", camOffsetYTopic.GetAtomic(0.0f).value);
    info("camheight: {}", camHeightTopic.GetAtomic(0.0f).value);
    info("camangle: {}", camAngleTopic.GetAtomic(0.0f).value);
}

/*! \brief Pushes a new coral distance value. User input is obtained through
 * cin, which pauses execution so the client may disconnect from the server for
 * a short time*/
void pushDistanceValue() {
    std::string value;
    info("Please enter a value: ");
    std::cin >> value;
    coralDistanceTopic.SetDefault(stof(value));
}

/*! \brief Pushes a new coral angle value. User input is obtained through cin,
 * which pauses execution so the client may disconnect from the server for a
 * short time*/
void pushAngleValue() {
    std::string value;
    info("Please enter a value: ");
    std::cin >> value;
    coralAngleTopic.SetDefault(stof(value));
}

int main(int argc, char** argv) {
    SetupTextLogger();
    signal(SIGINT, sighandler); // Handles Ctrl+C interrupts
    // Initialize the commands
    commandMap["help"] = showHelp;
    commandMap["stop"] = shutdownClient;
    commandMap["print"] = printNetworkTableValues;
    commandMap["pushdist"] = pushDistanceValue;
    commandMap["pushangle"] = pushAngleValue;
    nt::NetworkTableInstance netTableManager =
        nt::NetworkTableInstance::GetDefault();
    netTableManager.StartClient4("testnettableclient");
    netTableManager.SetServer("localhost", 5810U);
    auto objectDetectionTable = netTableManager.GetTable("ObjectDetection");
    // Initialize the topics
    hueMinTopic = objectDetectionTable->GetIntegerTopic("HueMin").Subscribe(0);
    hueMaxTopic = objectDetectionTable->GetIntegerTopic("HueMax").Subscribe(0);
    saturationMinTopic =
        objectDetectionTable->GetIntegerTopic("SaturationMin").Subscribe(0);
    saturationMaxTopic =
        objectDetectionTable->GetIntegerTopic("SaturationMax").Subscribe(0);
    valueMinTopic =
        objectDetectionTable->GetIntegerTopic("ValueMin").Subscribe(0);
    valueMaxTopic =
        objectDetectionTable->GetIntegerTopic("ValueMax").Subscribe(0);
    coralDistanceTopic =
        objectDetectionTable->GetDoubleTopic("CoralDistance").Publish();
    coralAngleTopic =
        objectDetectionTable->GetDoubleTopic("CoralAngle").Publish();
    minWidthTopic =
        objectDetectionTable->GetIntegerTopic("MinPixelWidth").Subscribe(0);
    maxWidthTopic =
        objectDetectionTable->GetIntegerTopic("MaxPixelWidth").Subscribe(0);
    minHeightTopic =
        objectDetectionTable->GetIntegerTopic("MinPixelHeight").Subscribe(0);
    maxHeightTopic =
        objectDetectionTable->GetIntegerTopic("MaxPixelHeight").Subscribe(0);
    minAreaTopic =
        objectDetectionTable->GetIntegerTopic("MinPixelArea").Subscribe(0);
    maxAreaTopic =
        objectDetectionTable->GetIntegerTopic("MaxPixelArea").Subscribe(0);
    minPercentFillTopic =
        objectDetectionTable->GetDoubleTopic("MinPercentFill").Subscribe(0.0f);
    camOffsetXTopic =
        objectDetectionTable->GetDoubleTopic("CamOffsetX").Subscribe(0.0f);
    camOffsetYTopic =
        objectDetectionTable->GetDoubleTopic("CamOffsetY").Subscribe(0.0f);
    camAngleTopic =
        objectDetectionTable->GetDoubleTopic("CamAngle").Subscribe(0.0f);
    camHeightTopic =
        objectDetectionTable->GetDoubleTopic("CamHeight").Subscribe(0.0f);
    filterWindowTopic =
        objectDetectionTable->GetIntegerTopic("FilterPacketWindow").Subscribe(0);
    showHelp(); // Print the available commands to the console
    auto commandInputThread = std::async(std::launch::async, [] {
        std::string input;
        std::cin >> input;
        return input;
    });
    while (!bStop) {
        // Print connection status
        info("Connection status of networktables: {}",
             netTableManager.IsConnected());
        if (readyForNewCmdThread) {
            // Launch an async thread to take user input so client can respond to any
            // server requests while waiting for input
            commandInputThread = std::async(std::launch::async, [] {
                std::string input;
                std::cin >> input;
                return input;
            });
            readyForNewCmdThread = false;
        }
        // If the async thread has gotten the input, process it
        if (commandInputThread.wait_for(0.5s) == std::future_status::ready) {
            std::string commandInput = commandInputThread.get();
            if (commandMap[commandInput] != NULL) {
                commandMap[commandInput]();
            } else {
                info("Command not recognized");
            }
            readyForNewCmdThread = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    info("Exiting...");
    netTableManager.StopClient();
    return 0;
}