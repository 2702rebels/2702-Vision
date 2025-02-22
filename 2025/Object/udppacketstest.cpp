/*
* This program serves to test the object detection packet serialziation and deserializing, as well as being able to send a test packet
* to an IP
* Depends on:
* - asio(UDP client library)
* - ObjectDetectionStruct.h and ObjectDetectionStruct.h
*/


#include <iostream>
#include "ObjectDetectionStruct.h"
#include "ObjectDetectionStruct.cpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <asio.hpp> // Include asio

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "27021"
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

using asio::ip::udp;

/* Serialize and send an object detection packet */
void sendObjectDetectionPacket(const ObjectDetectionPacket& packet, const std::string& host, const std::string& port) {
    try {
        asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint receiver_endpoint = *resolver.resolve(udp::v4(), host, port).begin();

        udp::socket socket(io_context);
        socket.open(udp::v4());

        // Serialize the packet
        std::vector<uint8_t> send_buf(ObjectDetectionPacketStructType::GetSize()); // Use the correct size!
        std::span<uint8_t> packetBytes(send_buf.data(), send_buf.size());
        ObjectDetectionPacketStructType::Pack(packetBytes, packet);

        // Send the data
        socket.send_to(asio::buffer(send_buf), receiver_endpoint);

        std::cout << "Packet sent to " << host << ":" << port << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception in sendObjectDetectionPacket: " << e.what() << std::endl;
    }
}

int main( int argc, char **argv ){
    std::cout << "test123\n";
    // Create object detections with testing data
    ObjectDetectionInfo coral1 = ObjectDetectionInfo(25.2f, 51.6f, 50, 0);
    ObjectDetectionInfo coral2 = ObjectDetectionInfo(50.2f, 24.3f, 15, 0);
    std::cout << "initialized detects successfully\n";
    // Initialize a span with a 1000 byte length(this should be compressed in production code), filled with 0s by default
    std::vector<uint8_t> coralBytesVec(1000);
    std::span<uint8_t> coralBytes(coralBytesVec.data(), coralBytesVec.size());
    StructTypeObjectDetectionInfo::Pack(coralBytes, coral1);
    ObjectDetectionInfo unpackedCoral = StructTypeObjectDetectionInfo::Unpack(coralBytes);
    std::cout << "unpackedCoral dist: " << unpackedCoral.m_distance << "\n";
    std::cout << "unpackedCoral angle: " << unpackedCoral.m_angle << "\n";
    std::cout << "unpackedCoral quality: " << unpackedCoral.m_quality << "\n";
    std::cout << "unpackedCoral type: " << unpackedCoral.m_type << "\n";
    if(unpackedCoral == coral1){
        std::cout << ":D coral serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "coral is sad :C" << "\n";
    }
    // Initialize a 1000-byte span, should be condensed in production code, filled with 0s by default
    std::vector<uint8_t> packetBytesVec(1000);
    std::span<uint8_t> packetBytes(packetBytesVec.data(), packetBytesVec.size());
    // Create a testing packet
    ObjectDetectionPacket packet(5, 1, 123123L, 2, coral1, coral2, ObjectDetectionInfo(), ObjectDetectionInfo());
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
    std::cout << "type1: " << unpackedPacket.m_coral1.m_type << "\n";
    std::cout << "distance2: " << unpackedPacket.m_coral2.m_distance << "\n";
    std::cout << "angle2: " << unpackedPacket.m_coral2.m_angle << "\n";
    std::cout << "quality2: " << unpackedPacket.m_coral2.m_quality << "\n";
    std::cout << "type2: " << unpackedPacket.m_coral2.m_type << "\n";
    std::cout << "distance: " << unpackedPacket.m_coral3.m_distance << "\n";
    std::cout << "angle3: " << unpackedPacket.m_coral3.m_angle << "\n";
    std::cout << "quality3: " << unpackedPacket.m_coral3.m_quality << "\n";
    std::cout << "type3: " << unpackedPacket.m_coral3.m_type << "\n";
    std::cout << "distance4: " << unpackedPacket.m_coral4.m_distance << "\n";
    std::cout << "angle4: " << unpackedPacket.m_coral4.m_angle << "\n";
    std::cout << "quality4: " << unpackedPacket.m_coral4.m_quality << "\n";
    std::cout << "type4: " << unpackedPacket.m_coral4.m_type << "\n";
    if(unpackedPacket == packet){
        std::cout << "packet serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "packet test failed :C" << "\n";
    }
    sendObjectDetectionPacket(packet, SERVER_IP, SERVER_PORT);
}