#include <iostream>
#include <networktables/NetworkTableInstance.h>
#include <networktables/NetworkTable.h>
#include <networktables/IntegerTopic.h>
#include <signal.h>
#include <chrono>
#include <thread>

bool bStop = false;

void sighandler(int nSig){
    printf("Received Signal: %d\n", nSig);
    bStop = true;
}

int main( int argc, char **argv ){
    signal(SIGINT, sighandler); // Handles Ctrl+C interrupts
    nt::NetworkTableInstance netTableManager = nt::NetworkTableInstance::GetDefault();
    netTableManager.StartServer();
    while(!bStop){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}