#include <iostream>
#include "udppackets.h"
#include "udppackets.cpp"
#include <string>
#include <sstream>
#include <iomanip>

using StructTypeCoralDetection = wpi::Struct<CoralDetection>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

int main( int argc, char **argv ){
    std::cout << "test123\n";
    CoralDetection coral1 = CoralDetection(25.0f, 51.6f, 50);
    CoralDetection coral2 = CoralDetection(50.0f, 24.3f, 15);
    std::cout << "initialized detects successfully\n";
    std::array<CoralDetection, 4> corals = {coral1, coral2};
    std::cout << "initialized detection array successfully\n";
    ObjectDetectionPacket packet(PACKET_KEY, 12324L, 1, 2, corals);
    std::cout << "initialized packet successfully\n";
    std::vector<uint8_t> bytesVec(1000);
    std::span<uint8_t> bytes(bytesVec.data(), bytesVec.size());
    std::cout << "initialized objects successfully\n";
    std::vector<uint8_t> coralBytesVec(1000);
    std::span<uint8_t> coralBytes(bytesVec.data(), bytesVec.size());
    StructTypeCoralDetection::Pack(bytes, coral1);
    std::stringstream ss1;
    ss1 << std::hex << std::uppercase << std::setfill('0');
    for(int i = 0; i < sizeof(bytes.data()); i++){
        ss1 << std::setw(2) << static_cast<int>(bytes.data()[i]);
    }
    std::cout << ss1.str() << "\n";
    CoralDetection unpackedCoral = StructTypeCoralDetection::Unpack(bytes);
    if(unpackedCoral == coral1){
        std::cout << ":D coral serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "unpackedCoral dist: " << unpackedCoral.m_distance << "\n";
        std::cout << "unpackedCoral angle: " << unpackedCoral.m_angle << "\n";
        std::cout << "unpackedCoral quality: " << unpackedCoral.m_quality << "\n";
        std::cout << "coral is sad :C" << "\n";
    }
    StructTypeObjectDetectionPacket::Pack(bytes, packet);
    /*std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for(int i = 0; i < sizeof(bytes.data()); i++){
        ss << std::setw(2) << static_cast<int>(bytes.data()[i]);
    }
    std::cout << ss.str() << "\n";*/
    std::cout << "packed successfully\n";
    ObjectDetectionPacket unpackedPacket = StructTypeObjectDetectionPacket::Unpack(bytes);
    std::cout << "key from unpacked: " << unpackedPacket.m_key << "\n";
    std::cout << "version from unpacked: " << unpackedPacket.m_versionNumber << "\n";
    std::cout << "time from unpacked: " << unpackedPacket.m_time << "\n";
    std::cout << "count from unpacked: " << unpackedPacket.m_count << "\n";
    for(CoralDetection detection : unpackedPacket.m_corals){
        std::cout << "distance for coral: " << detection.m_distance << "\n";
        std::cout << "angle for coral: " << detection.m_angle << "\n";
        std::cout << "quality for coral: " << detection.m_quality << "\n";
    }
    if(unpackedPacket == packet){
        std::cout << "test successful!\n";
    }
    else{
        std::cout << "Please send help :D \n";
    }
}