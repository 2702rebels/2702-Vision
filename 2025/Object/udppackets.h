#pragma once
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <vector>

#define PACKET_KEY 5
#define PACKET_KEY_SIZE 1
#define PACKET_VERSION_NUMBER 1
#define CORAL_DETECTION_SIZE_BYTES 12

class CoralDetection {
    public:
    CoralDetection(float distance, float angle, int quality);
    CoralDetection();
    float m_distance;
    float m_angle;
    int m_quality;
};

template <>
struct wpi::Struct<CoralDetection> {
    static constexpr std::string_view GetTypeName(){return "CoralDetection";}
    static constexpr size_t GetSize(){return 9;}
    static constexpr std::string_view GetSchema(){
        return "float distance;float angle;float quality";
    }
    static CoralDetection Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const CoralDetection& value);
};

static_assert(wpi::StructSerializable<CoralDetection>);

class ObjectDetectionPacket {
    public:
    ObjectDetectionPacket(int key, long t, int versionNumber, int count, std::array<CoralDetection, 4> corals);
    long m_time;
    int m_count;
    int m_key;
    int m_versionNumber;
    std::array<CoralDetection, 4> m_corals;
};

template <>
struct wpi::Struct<ObjectDetectionPacket> {
    static constexpr std::string_view GetTypeName(){return "ObjectDetectionPacket";}
    static constexpr size_t GetSize(){return 4 * 9 + 9 + 1 + 1 + 4;}
    static constexpr std::string_view GetSchema(){
        return "char[9] key;int versionNumber;long t;int count;CoralDetection[4] corals";
    }

    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket& value);
};

static_assert(wpi::StructSerializable<ObjectDetectionPacket>);