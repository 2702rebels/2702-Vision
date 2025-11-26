/*! \brief This programs acts as a UDP server to receive packets and decrypt them
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
using asio::ip::udp;

#define SERVER_PORT 27021
#define OBJECT_DETECTION_PACKET_SIZE 4 + 4 + 8 + 1 + 18 * 4
#define SIGINT 2

// This is the structure types where the serialization and deserialization functions are defined
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

std::vector<long> timestamps;
bool bStop = false;

void signalhandler(int nSig){
    std::cout << "Got exit signal: " << nSig << "\n";
    for(int i = 0; i < timestamps.size(); i++){
        std::cout << timestamps[i] << "\n";
    }
    bStop = true;
}

int main()
{
    signal(SIGINT, signalhandler);
    asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), SERVER_PORT));
    std::cout << "program booted" << "\n";

    while(!bStop)
    {
        std::array<char, OBJECT_DETECTION_PACKET_SIZE> receiving_buffer;
        udp::endpoint remoteEndpoint;
        size_t packetLen = socket.receive_from(asio::buffer(receiving_buffer), remoteEndpoint);
        auto bytesBuf = receiving_buffer.data();
        std::vector<uint8_t> packetVec(OBJECT_DETECTION_PACKET_SIZE);
        std::memcpy(packetVec.data(), bytesBuf, OBJECT_DETECTION_PACKET_SIZE);
        std::span<uint8_t> packetSpan(packetVec.data(), packetVec.size());
        std::cout << "Got packet" << "\n";
        std::cout << "packet len: " << packetVec.size() << "\n";
        ObjectDetectionPacket unpackedPacket = StructTypeObjectDetectionPacket::Unpack(packetSpan);
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
        timestamps.push_back(unpackedPacket.m_time);
    }
    return 0;
}