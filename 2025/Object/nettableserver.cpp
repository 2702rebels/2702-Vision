/*! \brief This is a program that acts as a network table server to receive and send object detection data for testing purposes. 
 * Depends on: 
 * - wpilib's ntcore
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

/*! \brief This is where all the commands are stored, in order to add a command, set a key(string) value to a void function like this:
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
nt::IntegerPublisher minWidthTopic;
nt::IntegerPublisher maxWidthTopic;
nt::IntegerPublisher minHeightTopic;
nt::IntegerPublisher maxHeightTopic;
nt::IntegerPublisher minAreaTopic;
nt::IntegerPublisher maxAreaTopic;
nt::DoublePublisher  minPercentFillTopic;
nt::DoublePublisher  camOffsetXTopic;
nt::DoublePublisher  camOffsetYTopic;
nt::DoublePublisher  camHeightTopic;
nt::DoublePublisher  camAngleTopic;
nt::IntegerPublisher filterWindowTopic;

/*! \brief This is called when Ctrl+C is pressed 
* \param nSig - The signal code that was interrupted, should always be 2 for Ctrl+C
*/
void sighandler(int nSig){
    printf("Received Signal: %d\n", nSig);
    std::cout << "Can't use Ctrl+C to exit, use stop command\n";
}

/*! \brief Prints the help text to console */
void showHelp(){
    std::cout << "Valid commands are: \n" << "print - logs all current topic values\n" << "pushhuemin - pushes a new value for the huemin param\n"
    << "pushhuemax - pushes a new value for the huemax param\n" << "pushsaturationmin - pushes a new value for the saturationmin param\n" << "pushsaturationmax - pushes a new value for the saturationmax param\n"
    << "pushvaluemin - pushes a new value for the valuemin param\n" << "pushvaluemax - pushes a new value for the valuemax param\n" << "pushminwidth - pushes a new value for the minpixelwidth param\n"
    << "pushmaxwidth - pushes a new value for the maxpixelwidth param\n" << "pushminheight - pushes a new value for the minpixelheight param\n" << "pushmaxheight - pushes a new value for the maxpixelheight\n" 
    << "pushminarea - pushes a new value for the minpixelarea param\n" << "pushmaxarea - pushes a new value for the maxpixelarea param\n" << "pushminpercentfill - pushes a new value for the min percent fill param\n" 
    << "pushcamoffsetx - pushes a new value for the camoffsetx param\n" << "pushcamoffsety - pushes a new value for the camoffsety param\n" << "pushcamheight - pushes a new value for the camheight param\n" 
    << "pushcamangle - pushes a new value for the camangle param\n" << "pushfilterwindow - pushes a new value for the filterwindow param\n" << "stop - shuts the server down\n" << "help - displays this text\n";
}

/*! \brief Shuts down the server. Note that this is not immediate, and the program will walk through one more tick before exiting.*/
void shutdown(){
    bStop = true;
}

/*! \brief Prints the network table values to console. This does not auto-update as more topics are added, so this should be updated when a new topic is added */
void printNetworkTableValues(){
    printf("Last received coral distance: %.6f\n",coralDistanceTopic.GetAtomic(0.0f).value);
    printf("Last received coral angle: %.6f\n",coralAngleTopic.GetAtomic(0.0f).value);
}

/*! \brief Pushes a new hue min value to the associated networktable topic. Value is provided through cin input */
void pushHueMin(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    hueMinTopic.SetDefault(stoi(value));
}

/*! \brief Pushes a new hue max value to the associated networktable topic. Value is provided through cin input */
void pushHueMax(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    hueMaxTopic.SetDefault(stoi(value));
}

/*! \brief Pushes a new saturation min value to the associated networktable topic. Value is provided through cin input */
void pushSaturationMin(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    saturationMinTopic.SetDefault(stoi(value));
}

/*! \brief Pushes a new saturation max value to the associated networktable topic. Value is provided through cin input */
void pushSaturationMax(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    saturationMaxTopic.SetDefault(stoi(value));
}

/*! \brief Pushes a new value min value to the associated networktable topic. Value is provided through cin input */
void pushValueMin(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    valueMinTopic.SetDefault(stoi(value));
}

/*! \brief Pushes a new value max value to the associated networktable topic. Value is provided through cin input */
void pushValueMax(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    valueMaxTopic.SetDefault(stoi(value));
}

/*! \brief Push a min pixel width. */
void pushMinWidth(){
    std::string value;
    std::cout << "Please enter a value in pixels: ";
    std::cin >> value;
    minWidthTopic.SetDefault(stoi(value));
}

/*! \brief Push a max pixel width. */
void pushMaxWidth(){
    std::string value;
    std::cout << "Please enter a value in pixels: ";
    std::cin >> value;
    maxWidthTopic.SetDefault(stoi(value));
}

/*! \brief Push a min pixel height. */
void pushMinHeight(){
    std::string value;
    std::cout << "Please enter a value in pixels: ";
    std::cin >> value;
    minHeightTopic.SetDefault(stoi(value));
}

/*! \brief Push a max pixel height. */
void pushMaxHeight(){
    std::string value;
    std::cout << "Please enter a value in pixels: ";
    std::cin >> value;
    maxHeightTopic.SetDefault(stoi(value));
}

/*! \brief Push a min pixel area. */
void pushMinArea(){
    std::string value;
    std::cout << "Please enter a value in pixels: ";
    std::cin >> value;
    minAreaTopic.SetDefault(stoi(value));
}

/*! \brief Push a max pixel area. */
void pushMaxArea(){
    std::string value;
    std::cout << "Please enter a value in pixels: ";
    std::cin >> value;
    maxAreaTopic.SetDefault(stoi(value));
}

/*! \brief Push a min percent fill. */
void pushMinPercentFill(){
    std::string value;
    std::cout << "Please enter a value as a float(eg 0.8): ";
    std::cin >> value;
    minPercentFillTopic.SetDefault(stod(value));
}

/*! \brief Push a cam offset X. */
void pushOffsetX(){
    std::string value;
    std::cout << "Please enter a value in meters: ";
    std::cin >> value;
    camOffsetXTopic.SetDefault(stod(value));
}

/*! \brief Push a cam offset Y. */
void pushOffsetY(){
    std::string value;
    std::cout << "Please enter a value in meters: ";
    std::cin >> value;
    camOffsetYTopic.SetDefault(stod(value));
}

/*! \brief Push a cam height. */
void pushCamHeight(){
    std::string value;
    std::cout << "Please enter a value in meters: ";
    std::cin >> value;
    camHeightTopic.SetDefault(stod(value));
}

/*! \brief Push a cam angle. */
void pushCamAngle(){
    std::string value;
    std::cout << "Please enter a value in radians: ";
    std::cin >> value;
    camAngleTopic.SetDefault(stod(value));
}

/*! \brief Push a new filter window. */
void pushFilterWindow(){
    std::string value;
    std::cout << "Please enter a value: ";
    std::cin >> value;
    filterWindowTopic.SetDefault(stoi(value));
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
    commandMap["pushminwidth"] = pushMinWidth;
    commandMap["pushmaxwidth"] = pushMaxWidth;
    commandMap["pushminheight"] = pushMinHeight;
    commandMap["pushmaxheight"] = pushMaxHeight;
    commandMap["pushminarea"] = pushMinArea;
    commandMap["pushmaxarea"] = pushMaxArea;
    commandMap["pushminpercentfill"] = pushMinPercentFill;
    commandMap["pushcamoffsetx"] = pushOffsetX;
    commandMap["pushcamoffsety"] = pushOffsetY;
    commandMap["pushcamheight"] = pushCamHeight;
    commandMap["pushcamangle"] = pushCamAngle;
    commandMap["pushfilterwindow"] = pushFilterWindow;
    nt::NetworkTableInstance netTableManager = nt::NetworkTableInstance::GetDefault();
    netTableManager.StartServer(); // Server will accept requests using either the NT3 or NT4 protocols
    auto objectDetectionTable = netTableManager.GetTable("ObjectDetection");
    // Initialize the topics
    hueMinTopic = objectDetectionTable->GetIntegerTopic("HueMin").Publish();
    hueMaxTopic = objectDetectionTable->GetIntegerTopic("HueMax").Publish();
    saturationMinTopic = objectDetectionTable->GetIntegerTopic("SaturationMin").Publish();
    saturationMaxTopic = objectDetectionTable->GetIntegerTopic("SaturationMax").Publish();
    valueMinTopic = objectDetectionTable->GetIntegerTopic("ValueMin").Publish();
    valueMaxTopic = objectDetectionTable->GetIntegerTopic("ValueMax").Publish();
    coralDistanceTopic = objectDetectionTable->GetDoubleTopic("CoralDistance").Subscribe(0.0f);
    coralAngleTopic = objectDetectionTable->GetDoubleTopic("CoralAngle").Subscribe(0.0f);
    minWidthTopic = objectDetectionTable->GetIntegerTopic("MinPixelWidth").Publish();
    maxHeightTopic = objectDetectionTable->GetIntegerTopic("MaxPixelWidth").Publish();
    minHeightTopic = objectDetectionTable->GetIntegerTopic("MinPixelHeight").Publish();
    maxHeightTopic = objectDetectionTable->GetIntegerTopic("MaxPixelHeight").Publish();
    minAreaTopic = objectDetectionTable->GetIntegerTopic("MinPixelArea").Publish();
    maxAreaTopic = objectDetectionTable->GetIntegerTopic("MaxPixelArea").Publish();
    minPercentFillTopic = objectDetectionTable->GetDoubleTopic("MinPercentFill").Publish();
    camOffsetXTopic = objectDetectionTable->GetDoubleTopic("CamOffsetX").Publish();
    camOffsetYTopic = objectDetectionTable->GetDoubleTopic("CamOffsetY").Publish();
    camHeightTopic = objectDetectionTable->GetDoubleTopic("CamHeight").Publish();
    camAngleTopic = objectDetectionTable->GetDoubleTopic("CamAngle").Publish();
    filterWindowTopic = objectDetectionTable->GetIntegerTopic("FilterPacketWindow").Publish();
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