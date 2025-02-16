#include <iostream>
#include "ObjectDetectionStruct.h"
#include "ObjectDetectionStruct.cpp"
#include <string>
#include <sstream>
#include <iomanip>

using StructTypeCoralDetection = wpi::Struct<CoralDetection>;

int main( int argc, char **argv ){
    std::cout << "test123\n";
    CoralDetection coral1 = CoralDetection(25.0f, 51.6f, 50);
    CoralDetection coral2 = CoralDetection(50.0f, 24.3f, 15);
    std::cout << "initialized detects successfully\n";
    std::array<CoralDetection, 4> corals = {coral1, coral2};
    std::cout << "initialized detection array successfully\n";
    std::cout << "initialized objects successfully\n";
    std::vector<uint8_t> coralBytesVec(1000);
    std::span<uint8_t> coralBytes(coralBytesVec.data(), coralBytesVec.size());
    StructTypeCoralDetection::Pack(coralBytes, coral1);
    std::stringstream ss1;
    ss1 << std::hex << std::uppercase << std::setfill('0');
    for(int i = 0; i < sizeof(coralBytes.data()); i++){
        ss1 << std::setw(2) << static_cast<int>(coralBytes.data()[i]);
    }
    std::cout << ss1.str() << "\n";
    CoralDetection unpackedCoral = StructTypeCoralDetection::Unpack(coralBytes);
    if(unpackedCoral.m_distance == coral1.m_distance && unpackedCoral.m_angle == coral1.m_angle && unpackedCoral.m_quality == coral1.m_quality){
        std::cout << ":D coral serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "unpackedCoral dist: " << unpackedCoral.m_distance << "\n";
        std::cout << "unpackedCoral angle: " << unpackedCoral.m_angle << "\n";
        std::cout << "unpackedCoral quality: " << unpackedCoral.m_quality << "\n";
        std::cout << "coral is sad :C" << "\n";
    }
}