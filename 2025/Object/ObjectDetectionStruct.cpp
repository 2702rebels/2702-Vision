#include "ObjectDetectionStruct.h"
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

namespace
{
    constexpr size_t typeOff = 0;
    constexpr size_t distOff = typeOff + 1;
    constexpr size_t angleOff = distOff + 8;
    constexpr size_t qualityOff = angleOff + 8;
    constexpr size_t keyOff = 0;
    constexpr size_t versionOff = keyOff + 4;
    constexpr size_t timeOff = versionOff + 4;
    constexpr size_t countOff = timeOff + 8;
    constexpr size_t coral1Off = countOff + 1;
    constexpr size_t coral2Off = coral1Off + 18;
    constexpr size_t coral3Off = coral2Off + 18;
    constexpr size_t coral4Off = coral3Off + 18;
} // namespace

ObjectDetectionInfo::ObjectDetectionInfo(double distance, double angle, uint8_t quality, uint8_t type) : m_distance(distance), m_angle(angle), m_quality(quality), m_type(type) {}

ObjectDetectionInfo::ObjectDetectionInfo()
{
    m_distance = 0.0f;
    m_angle = 0.0f;
    m_quality = 0;
    m_type = 0;
}

using ObjectDetectionInfoStructType = wpi::Struct<ObjectDetectionInfo>;

ObjectDetectionInfo ObjectDetectionInfoStructType::Unpack(std::span<const uint8_t> data)
{
    uint8_t type = wpi::UnpackStruct<uint8_t, typeOff>(data);
    double dist = wpi::UnpackStruct<double, distOff>(data);
    double angle = wpi::UnpackStruct<double, angleOff>(data);
    uint8_t quality = wpi::UnpackStruct<int, qualityOff>(data);
    return ObjectDetectionInfo(dist, angle, quality, type);
}

void ObjectDetectionInfoStructType::Pack(std::span<uint8_t> data, const ObjectDetectionInfo &value)
{
    wpi::PackStruct<typeOff>(data, value.m_type);
    wpi::PackStruct<distOff>(data, value.m_distance);
    wpi::PackStruct<angleOff>(data, value.m_angle);
    wpi::PackStruct<qualityOff>(data, value.m_quality);
}

ObjectDetectionPacket::ObjectDetectionPacket()
{
    m_key = 0;
    m_versionNumber = -1;
    m_time = -1L;
    m_count = 0;
    m_coral1 = ObjectDetectionInfo();
    m_coral2 = ObjectDetectionInfo();
    m_coral3 = ObjectDetectionInfo();
    m_coral4 = ObjectDetectionInfo();
}

ObjectDetectionPacket::ObjectDetectionPacket(int key, int versionNumber, long timestamp, uint8_t count, ObjectDetectionInfo coral1, ObjectDetectionInfo coral2, ObjectDetectionInfo coral3, ObjectDetectionInfo coral4) 
: m_key(key), m_versionNumber(versionNumber), m_time(timestamp), m_count(count), m_coral1(coral1), m_coral2(coral2), m_coral3(coral3), m_coral4(coral4) {}

using ObjectDetectionPacketStructType = wpi::Struct<ObjectDetectionPacket>;

ObjectDetectionPacket ObjectDetectionPacketStructType::Unpack(std::span<const uint8_t> data)
{
    int key = wpi::UnpackStruct<int, keyOff>(data);
    int versionNumber = wpi::UnpackStruct<int, versionOff>(data);
    long time = wpi::UnpackStruct<long, timeOff>(data);
    uint8_t count = wpi::UnpackStruct<uint8_t, countOff>(data);
    ObjectDetectionInfo coral1 = wpi::UnpackStruct<ObjectDetectionInfo, coral1Off>(data);
    ObjectDetectionInfo coral2 = wpi::UnpackStruct<ObjectDetectionInfo, coral2Off>(data);
    ObjectDetectionInfo coral3 = wpi::UnpackStruct<ObjectDetectionInfo, coral3Off>(data);
    ObjectDetectionInfo coral4 = wpi::UnpackStruct<ObjectDetectionInfo, coral4Off>(data);
    return ObjectDetectionPacket(key, versionNumber, time, count, coral1, coral2, coral3, coral4);
}

void ObjectDetectionPacketStructType::Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value)
{
    wpi::PackStruct<keyOff>(data, value.m_key);
    wpi::PackStruct<versionOff>(data, value.m_versionNumber);
    wpi::PackStruct<timeOff>(data, value.m_time);
    wpi::PackStruct<countOff>(data, value.m_count);
    wpi::PackStruct<coral1Off>(data, value.m_coral1);
    wpi::PackStruct<coral2Off>(data, value.m_coral2);
    wpi::PackStruct<coral3Off>(data, value.m_coral3);
    wpi::PackStruct<coral4Off>(data, value.m_coral4);
}