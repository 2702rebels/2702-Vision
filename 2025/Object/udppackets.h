#pragma once
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <vector>

#define PACKET_KEY 5
#define PACKET_KEY_SIZE 1
#define PACKET_VERSION_NUMBER 1
#define CORAL_DETECTION_SIZE_BYTES 12

class CoralDetection
{
public:
    CoralDetection(float distance, float angle, int quality);
    CoralDetection();
    float m_distance;
    float m_angle;
    int m_quality;

    bool operator==(const CoralDetection &c)
    {
        if (m_distance == c.m_distance && m_angle == c.m_angle && m_quality == c.m_quality){
            return true;
        }
        return false;
    }
};

template <>
struct wpi::Struct<CoralDetection>
{
    static constexpr std::string_view GetTypeName() { return "CoralDetection"; }
    static constexpr size_t GetSize() { return 17; }
    static constexpr std::string_view GetSchema()
    {
        return "float distance;float angle;int quality";
    }
    static CoralDetection Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const CoralDetection &value);
};

static_assert(wpi::StructSerializable<CoralDetection>);

class ObjectDetectionPacket
{
public:
    ObjectDetectionPacket(int key, long t, int versionNumber, int count, std::array<CoralDetection, 4> corals);
    long m_time;
    int m_count;
    int m_key;
    int m_versionNumber;
    std::array<CoralDetection, 4> m_corals;

    bool operator==(const ObjectDetectionPacket &c)
    {
        if (m_time == c.m_time && m_count == c.m_count && m_key == c.m_key && m_versionNumber == c.m_versionNumber && 
            m_corals[0] == c.m_corals[0] && m_corals[1] == c.m_corals[1] && m_corals[2] == c.m_corals[2] && m_corals[3] == c.m_corals[3]){
            return true;
        }
        return false;
    }
};

template <>
struct wpi::Struct<ObjectDetectionPacket>
{
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionPacket"; }
    static constexpr size_t GetSize() { return 1 + 1 + 4 + 1 + 4 * 9; }
    static constexpr std::string_view GetSchema()
    {
        return "int key;int versionNumber;long t;int count;CoralDetection coral1;CoralDetection coral2;CoralDetection coral3;CoralDetection coral4";
    }

    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value);
};

static_assert(wpi::StructSerializable<ObjectDetectionPacket>);