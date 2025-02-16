#include "ObjectDetectionStruct.h"
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

namespace {
constexpr size_t distOff = 0;
constexpr size_t angleOff = distOff + 4;
constexpr size_t qualityOff = angleOff + 4;
constexpr size_t keyOff = 0;
constexpr size_t versionOff = keyOff + 1;
constexpr size_t timeOff = versionOff + 1;
constexpr size_t countOff = timeOff + 4;
constexpr size_t coral1Off = countOff + 1;
constexpr size_t coral2Off = coral1Off + 9;
constexpr size_t coral3Off = coral2Off + 9;
constexpr size_t coral4Off = coral3Off + 9;
}  // namespace

CoralDetection::CoralDetection(float distance, float angle, int quality) : m_distance(distance), m_angle(angle), m_quality(quality){}

CoralDetection::CoralDetection(){
    m_distance = 0.0f;
    m_angle = 0.0f;
    m_quality = 0;
}

using CoralDetectionStructType = wpi::Struct<CoralDetection>;

CoralDetection CoralDetectionStructType::Unpack(std::span<const uint8_t> data) {
    float dist = wpi::UnpackStruct<float, distOff>(data);
    float angle = wpi::UnpackStruct<float, angleOff>(data);
    int quality = wpi::UnpackStruct<int, qualityOff>(data);
    return CoralDetection(dist, angle, quality);
}

void CoralDetectionStructType::Pack(std::span<uint8_t> data, const CoralDetection& value) {
  wpi::PackStruct<distOff>(data, value.m_distance);
  wpi::PackStruct<angleOff>(data, value.m_angle);
  wpi::PackStruct<qualityOff>(data, value.m_quality);
}

ObjectDetectionPacket::ObjectDetectionPacket(){
    m_key = 0;
    m_versionNumber = -1;
    m_time = -1L;
    m_count = -1;
    m_coral1 = CoralDetection();
    m_coral2 = CoralDetection();
    m_coral3 = CoralDetection();
    m_coral4 = CoralDetection();
}

ObjectDetectionPacket::ObjectDetectionPacket(int key, int versionNumber, long time, int count, CoralDetection coral1, CoralDetection coral2, CoralDetection coral3, CoralDetection coral4):
m_key(key), m_versionNumber(versionNumber), m_time(time), m_count(count), m_coral1(coral1), m_coral2(coral2), m_coral3(coral3), m_coral4(coral4){}

using ObjectDetectionPacketStructType = wpi::Struct<ObjectDetectionPacket>;

ObjectDetectionPacket ObjectDetectionPacketStructType::Unpack(std::span<const uint8_t> data) {
    int key = wpi::UnpackStruct<int, keyOff>(data);
    int versionNumber = wpi::UnpackStruct<int, versionOff>(data);
    long time = wpi::UnpackStruct<long, timeOff>(data);
    int count = wpi::UnpackStruct<int, countOff>(data);
    CoralDetection coral1 = wpi::UnpackStruct<CoralDetection, coral1Off>(data);
    CoralDetection coral2 = wpi::UnpackStruct<CoralDetection, coral2Off>(data);
    CoralDetection coral3 = wpi::UnpackStruct<CoralDetection, coral3Off>(data);
    CoralDetection coral4 = wpi::UnpackStruct<CoralDetection, coral4Off>(data);
    return ObjectDetectionPacket(key, versionNumber, time, count, coral1, coral2, coral3, coral4);
}

void ObjectDetectionPacketStructType::Pack(std::span<uint8_t> data, const ObjectDetectionPacket& value) {
  wpi::PackStruct<keyOff>(data, value.m_key);
  wpi::PackStruct<versionOff>(data, value.m_versionNumber);
  wpi::PackStruct<timeOff>(data, value.m_time);
  wpi::PackStruct<countOff>(data, value.m_count);
  wpi::PackStruct<coral1Off>(data, value.m_coral1);
  wpi::PackStruct<coral2Off>(data, value.m_coral2);
  wpi::PackStruct<coral3Off>(data, value.m_coral3);
  wpi::PackStruct<coral4Off>(data, value.m_coral4);
}