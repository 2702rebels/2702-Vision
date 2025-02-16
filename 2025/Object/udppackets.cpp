#include "udppackets.h"
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

namespace
{
    constexpr size_t coralDetectDistOff = 0;
    constexpr size_t coralDetectAngleOff = coralDetectDistOff + 8;
    constexpr size_t coralDetectQualityOff = coralDetectAngleOff + 8;
    constexpr size_t packetKeyOff = 0;
    constexpr size_t versionNumberOff = packetKeyOff + PACKET_KEY_SIZE;
    constexpr size_t timeOff = versionNumberOff + 1;
    constexpr size_t countOff = timeOff + 4;
    constexpr size_t detect1Off = countOff + 1;
    constexpr size_t detect2Off = detect1Off + 9;
    constexpr size_t detect3Off = detect2Off + 9;
    constexpr size_t detect4Off = detect3Off + 9;
}

using StructTypeCoralDetection = wpi::Struct<CoralDetection>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

CoralDetection StructTypeCoralDetection::Unpack(std::span<const uint8_t> data)
{
    return CoralDetection(
        wpi::UnpackStruct<float, coralDetectQualityOff>(data),
        wpi::UnpackStruct<float, coralDetectAngleOff>(data),
        wpi::UnpackStruct<uint8_t, coralDetectQualityOff>(data));
}

void StructTypeCoralDetection::Pack(std::span<uint8_t> data, const CoralDetection &value)
{
    wpi::PackStruct<coralDetectDistOff>(data, value.m_distance);
    wpi::PackStruct<coralDetectAngleOff>(data, value.m_angle);
    wpi::PackStruct<coralDetectQualityOff>(data, value.m_quality);
}

ObjectDetectionPacket StructTypeObjectDetectionPacket::Unpack(std::span<const uint8_t> data)
{
    uint8_t key = wpi::UnpackStruct<uint8_t, packetKeyOff>(data);
    uint8_t versionNumber = wpi::UnpackStruct<uint8_t, versionNumberOff>(data);
    std::array<CoralDetection, 4> corals;
    corals[0] = wpi::UnpackStruct<CoralDetection, detect1Off>(data);
    corals[1] = wpi::UnpackStruct<CoralDetection, detect2Off>(data);
    corals[2] = wpi::UnpackStruct<CoralDetection, detect3Off>(data);
    corals[3] = wpi::UnpackStruct<CoralDetection, detect4Off>(data);
    std::span<const uint8_t> subspan(data.begin() + timeOff, data.begin() + timeOff + 3);
    std::cout << std::string(subspan.begin(), subspan.end()) << "\n";
    long testTime;
    memcpy(&testTime, subspan.data(), sizeof(subspan.data()));
    return ObjectDetectionPacket(key, testTime, versionNumber, wpi::UnpackStruct<uint8_t, countOff>(data),
                                 corals);
}

void StructTypeObjectDetectionPacket::Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value)
{
    wpi::PackStruct<packetKeyOff>(data, value.m_key);
    wpi::PackStruct<versionNumberOff>(data, value.m_versionNumber);
    // pack long
    data[timeOff] = (int)((value.m_time >> 24) & 0xFF);
    data[timeOff + 1] = (int)((value.m_time >> 16) & 0xFF);
    data[timeOff + 2] = (int)((value.m_time >> 8) & 0XFF);
    data[timeOff + 3] = (int)((value.m_time & 0XFF));
    wpi::PackStruct<countOff>(data, value.m_count);
    wpi::PackStruct<detect1Off>(data, value.m_corals[0]);
    wpi::PackStruct<detect2Off>(data, value.m_corals[1]);
    wpi::PackStruct<detect3Off>(data, value.m_corals[2]);
    wpi::PackStruct<detect4Off>(data, value.m_corals[3]);
}

CoralDetection::CoralDetection(float distance, float angle, int quality)
{
    m_distance = distance;
    m_angle = angle;
    m_quality = quality;
}

CoralDetection::CoralDetection(){
    m_distance = 0.0f;
    m_angle = 0.0f;
    m_quality = 0;
}

ObjectDetectionPacket::ObjectDetectionPacket(int key, long t, int versionNumber, int count, std::array<CoralDetection, 4> corals)
{
    m_key = key;
    m_time = t;
    m_versionNumber = versionNumber;
    m_count = count;
    m_corals = corals;
}