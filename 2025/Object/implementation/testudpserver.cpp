/*! \brief This programs acts as a UDP server to receive packets and decrypt
 * them Depends on:
 * - asio(UDP client library)
 * - ObjectDetectionStruct.h and ObjectDetectionStruct.h
 */

#include "LoggingUtils.h"
#include "ObjectDetectionStruct.h"

#include <asio.hpp> // Include asio
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
using asio::ip::udp;

enum {
SERVER_PORT = 27021,
OBJECT_DETECTION_PACKET_SIZE = (4 + 4 + 8 + 1 + 18 * 4),
SIGINT = 2
};

// This is the structure types where the serialization and deserialization
// functions are defined
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

std::vector<long> timestamps;
bool bStop = false;

void Signalhandler(int nSig) {
    info("Got exit signal: {}", nSig);
    for (long & timestamp : timestamps) {
        info("{}", timestamp);
    }
    bStop = true;
}

int main() {
    SetupTextLogger();
    signal(SIGINT, Signalhandler);
    asio::io_context ioContext;
    udp::socket socket(ioContext, udp::endpoint(udp::v4(), SERVER_PORT));
    info("program booted");

    while (!bStop) {
        std::array<char, OBJECT_DETECTION_PACKET_SIZE> receivingBuffer{};
        udp::endpoint remoteEndpoint;
        size_t packetLen =
            socket.receive_from(asio::buffer(receivingBuffer), remoteEndpoint);
        auto bytesBuf = receivingBuffer.data();
        std::vector<uint8_t> packetVec(OBJECT_DETECTION_PACKET_SIZE);
        std::memcpy(packetVec.data(), bytesBuf, OBJECT_DETECTION_PACKET_SIZE);
        std::span<uint8_t> packetSpan(packetVec.data(), packetVec.size());
        info("Got packet");
        info("packet len: {}", packetVec.size());
        ObjectDetectionPacket unpackedPacket =
            StructTypeObjectDetectionPacket::Unpack(packetSpan);
        info("unpacked properties:");
        info("key: {}", unpackedPacket.m_key);
        info("version: {}", unpackedPacket.m_versionNumber);
        info("time: {}", unpackedPacket.m_time);
        info("count: {}", unpackedPacket.m_count);
        info("distance1: {}", unpackedPacket.m_coral1.m_distance);
        info("angle1: {}", unpackedPacket.m_coral1.m_angle);
        info("quality1: {}", unpackedPacket.m_coral1.m_quality);
        info("type1: {}", static_cast<uint8_t>(unpackedPacket.m_coral1.m_type));
        info("distance2: {}", unpackedPacket.m_coral2.m_distance);
        info("angle2: {}", unpackedPacket.m_coral2.m_angle);
        info("quality2: {}", unpackedPacket.m_coral2.m_quality);
        info("type2: {}", static_cast<uint8_t>(unpackedPacket.m_coral2.m_type));
        info("distance: {}", unpackedPacket.m_coral3.m_distance);
        info("angle3: {}", unpackedPacket.m_coral3.m_angle);
        info("quality3: {}", unpackedPacket.m_coral3.m_quality);
        info("type3: {}", static_cast<uint8_t>(unpackedPacket.m_coral3.m_type));
        info("distance4: {}", unpackedPacket.m_coral4.m_distance);
        info("angle4: {}", unpackedPacket.m_coral4.m_angle);
        info("quality4: {}", unpackedPacket.m_coral4.m_quality);
        info("type4: {}", static_cast<uint8_t>(unpackedPacket.m_coral4.m_type));
        timestamps.push_back(unpackedPacket.m_time);
    }
    return 0;
}