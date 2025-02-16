#include <iostream>
#include "ObjectDetectionStruct.h"
#include "ObjectDetectionStruct.cpp"
#include <string>
#include <sstream>
#include <iomanip>

using StructTypeCoralDetection = wpi::Struct<CoralDetection>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

int main( int argc, char **argv ){
    std::cout << "test123\n";
    CoralDetection coral1 = CoralDetection(25.2f, 51.6f, 50);
    CoralDetection coral2 = CoralDetection(50.2f, 24.3f, 15);
    std::cout << "initialized detects successfully\n";
    std::vector<uint8_t> coralBytesVec(1000);
    std::span<uint8_t> coralBytes(coralBytesVec.data(), coralBytesVec.size());
    StructTypeCoralDetection::Pack(coralBytes, coral1);
    CoralDetection unpackedCoral = StructTypeCoralDetection::Unpack(coralBytes);
    std::cout << "unpackedCoral dist: " << unpackedCoral.m_distance << "\n";
    std::cout << "unpackedCoral angle: " << unpackedCoral.m_angle << "\n";
    std::cout << "unpackedCoral quality: " << unpackedCoral.m_quality << "\n";
    if(unpackedCoral.m_distance == coral1.m_distance && unpackedCoral.m_angle == coral1.m_angle && unpackedCoral.m_quality == coral1.m_quality){
        std::cout << ":D coral serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "coral is sad :C" << "\n";
    }
    std::vector<uint8_t> packetBytesVec(1000);
    std::span<uint8_t> packetBytes(packetBytesVec.data(), packetBytesVec.size());
    ObjectDetectionPacket packet(5, 1, 123123L, 2, coral1, coral2, CoralDetection(), CoralDetection());
    StructTypeObjectDetectionPacket::Pack(packetBytes, packet);
    ObjectDetectionPacket unpackedPacket = StructTypeObjectDetectionPacket::Unpack(packetBytes);
    std::cout << "unpacked properties:" << "\n";
    std::cout << "key: " << unpackedPacket.m_key << "\n";
    std::cout << "version: " << unpackedPacket.m_versionNumber << "\n";
    std::cout << "time: " << unpackedPacket.m_time << "\n";
    std::cout << "count: " << unpackedPacket.m_count << "\n";
    std::cout << "distance1: " << unpackedPacket.m_coral1.m_distance << "\n";
    std::cout << "angle1: " << unpackedPacket.m_coral1.m_angle << "\n";
    std::cout << "quality1: " << unpackedPacket.m_coral1.m_quality << "\n";
    std::cout << "distance2: " << unpackedPacket.m_coral2.m_distance << "\n";
    std::cout << "angle2: " << unpackedPacket.m_coral2.m_angle << "\n";
    std::cout << "quality2: " << unpackedPacket.m_coral2.m_quality << "\n";
    std::cout << "distance: " << unpackedPacket.m_coral3.m_distance << "\n";
    std::cout << "angle3: " << unpackedPacket.m_coral3.m_angle << "\n";
    std::cout << "quality3: " << unpackedPacket.m_coral3.m_quality << "\n";
    std::cout << "distance4: " << unpackedPacket.m_coral4.m_distance << "\n";
    std::cout << "angle4: " << unpackedPacket.m_coral4.m_angle << "\n";
    std::cout << "quality4: " << unpackedPacket.m_coral4.m_quality << "\n";
    if(unpackedPacket.m_key == packet.m_key && unpackedPacket.m_count == packet.m_count && unpackedPacket.m_time == packet.m_time && unpackedPacket.m_versionNumber == packet.m_versionNumber && unpackedPacket.m_coral1.m_distance == packet.m_coral1.m_distance && unpackedPacket.m_coral1.m_angle == packet.m_coral1.m_angle && unpackedPacket.m_coral1.m_quality == packet.m_coral1.m_quality && unpackedPacket.m_coral2.m_distance == packet.m_coral2.m_distance && unpackedPacket.m_coral2.m_angle == packet.m_coral2.m_angle && unpackedPacket.m_coral2.m_quality == packet.m_coral2.m_quality && unpackedPacket.m_coral3.m_distance == packet.m_coral3.m_distance && unpackedPacket.m_coral3.m_angle == packet.m_coral3.m_angle && unpackedPacket.m_coral3.m_quality == packet.m_coral3.m_quality && unpackedPacket.m_coral4.m_distance == packet.m_coral4.m_distance && unpackedPacket.m_coral4.m_angle == packet.m_coral4.m_angle && unpackedPacket.m_coral4.m_quality == packet.m_coral4.m_quality){
        std::cout << "packet serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "packet test failed :C" << "\n";
    }
}