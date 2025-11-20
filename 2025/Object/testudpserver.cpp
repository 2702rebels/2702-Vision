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
#include "LoggingUtils.h"
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
    info("Got exit signal: {}", nSig);
    for(int i = 0; i < timestamps.size(); i++){
        info("{}", timestamps[i]);
    }
    bStop = true;
}

int main()
{
    SetupTextLogger();
    signal(SIGINT, signalhandler);
    asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), SERVER_PORT));
    info("program booted");

    while(!bStop)
    {
        std::array<char, OBJECT_DETECTION_PACKET_SIZE> receiving_buffer;
        udp::endpoint remoteEndpoint;
        size_t packetLen = socket.receive_from(asio::buffer(receiving_buffer), remoteEndpoint);
        auto bytesBuf = receiving_buffer.data();
        std::vector<uint8_t> packetVec(OBJECT_DETECTION_PACKET_SIZE);
        std::memcpy(packetVec.data(), bytesBuf, OBJECT_DETECTION_PACKET_SIZE);
        std::span<uint8_t> packetSpan(packetVec.data(), packetVec.size());
        info("Got packet");
        info("packet len: {}", packetVec.size());
        ObjectDetectionPacket unpackedPacket = StructTypeObjectDetectionPacket::Unpack(packetSpan);
        info("unpacked properties:");
        info("key: {}", unpackedPacket.m_key);
        info("version: {}", unpackedPacket.m_versionNumber);
        info("time: {}", unpackedPacket.m_time);
        info("count: {}", unpackedPacket.m_count);
        info("distance1: {}", unpackedPacket.m_coral1.m_distance);
        info("angle1: {}", unpackedPacket.m_coral1.m_angle);
        info("quality1: {}", unpackedPacket.m_coral1.m_quality);
        info("type1: {}", unpackedPacket.m_coral1.m_type);
        info("distance2: {}", unpackedPacket.m_coral2.m_distance);
        info("angle2: {}", unpackedPacket.m_coral2.m_angle);
        info("quality2: {}", unpackedPacket.m_coral2.m_quality);
        info("type2: {}", unpackedPacket.m_coral2.m_type);
        info("distance: {}", unpackedPacket.m_coral3.m_distance);
        info("angle3: {}", unpackedPacket.m_coral3.m_angle);
        info("quality3: {}", unpackedPacket.m_coral3.m_quality);
        info("type3: {}", unpackedPacket.m_coral3.m_type);
        info("distance4: {}", unpackedPacket.m_coral4.m_distance);
        info("angle4: {}", unpackedPacket.m_coral4.m_angle);
        info("quality4: {}", unpackedPacket.m_coral4.m_quality);
        info("type4: {}", unpackedPacket.m_coral4.m_type);
        timestamps.push_back(unpackedPacket.m_time);
    }
    return 0;
}