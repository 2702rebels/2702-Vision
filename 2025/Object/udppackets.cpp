#include "udppackets.h"
#include <string>
#include <span>
#include <Struct.h>

namespace {
    constexpr size_t coralDetectDistOff = 0;
    constexpr size_t coralDetectAngleOff = coralDetectDistOff + 4;
    constexpr size_t coralDetectQualityOff = coralDetectAngleOff + 4;
    constexpr size_t packetKeyOff = 0;
    constexpr size_t versionNumberOff = packetKeyOff + PACKET_KEY_SIZE;
    constexpr size_t timeOff = versionNumberOff + 1;
    constexpr size_t countOff = timeOff + 4;
    constexpr size_t detectsOff = countOff + 1;
}
CoralDetection::CoralDetection(float distance, float angle, int quality) : m_distance(distance), m_angle(angle), m_quality(quality){}

CoralDetection StructType::Unpack(std::span<const uint8_t> data){
    return CoralDetection(
        wpi::UnpackStruct<float, coralDetectQualityOff>(data),
        wpi::UnpackStruct<float, coralDetectAngleOff>(data),
        wpi::UnpackStruct<int, coralDetectQualityOff>(data)
    );
}

void StructType::Pack(std::span<uint8_t> data, const CoralDetection& value){
    wpi::PackStruct<coralDetectDistOff>(data, value.m_distance);
    wpi::PackStruct<coralDetectAngleOff>(data, value.m_angle);
    wpi::PackStruct<coralDetectQualityOff>(data, value.m_quality);
}

ObjectDetectionPacket::ObjectDetectionPacket(std::string key, long t, int versionNumber, int count, CoralDetection corals[4]):
    m_key(key), m_t(t), m_versionNumber(versionNumber), m_count(count), m_corals(corals){}


ObjectDetectionPacket StructType::Unpack(std::span<const uint8_t> data){
    int key = wpi::UnpackStruct<int, packetKeyOff>(data);
    if(key != PACKET_KEY){
        std::cout << "Invalid key!!!\n";
        return nullptr;
    }
    int versionNumber = wpi::UnpackStruct<int, versionNumberOff>(data);
    if(versionNumber != PACKET_VERSION_NUMBER){
        std::cout << "Wrong version!!!\n";
        return nullptr;
    }
    return ObjectDetectionPacket(key, versionNumber, wpi::UnpackStruct<long, timeOff>(data),
            wpi::UnpackStruct<int, countOff>(data), wpi::Unpack<CoralDetection[4], detectsOff>());
}

void StructType::Pack(std::span<uint8_t> data, const ObjectDetectionPacket& value){
    wpi::PackStruct<packetKeyOff>(data, value.m_key);
    wpi::PackStruct<versionNumberOff>(data, value.m_versionNumber);
    wpi::PackStruct<timeOff>(data, value.m_t);
    wpi::PackStruct<countOff>(data, value.m_count);
    wpi::PackStruct<detectsOff>(data, value.m_corals);
}