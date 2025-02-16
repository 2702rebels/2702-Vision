#pragma once
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

class CoralDetection
{
public:
    CoralDetection();
    CoralDetection(float distance, float angle, int quality);
    float m_distance;
    float m_angle;
    int m_quality;
};

template <>
struct wpi::Struct<CoralDetection>
{
    static constexpr std::string_view GetTypeName() { return "CoralDetection"; }
    static constexpr size_t GetSize() { return 12; }
    static constexpr std::string_view GetSchema()
    {
        return "float m_distance;float m_angle;int m_quality";
    }

    static CoralDetection Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const CoralDetection &value);
};

static_assert(wpi::StructSerializable<CoralDetection>);

class ObjectDetectionPacket
{
public:
    ObjectDetectionPacket();
    ObjectDetectionPacket(int key, int versionNumber, long time, int count, CoralDetection coral1, CoralDetection coral2, CoralDetection coral3, CoralDetection coral4);
    int m_key;
    int m_versionNumber;
    long m_time;
    int m_count;
    CoralDetection m_coral1;
    CoralDetection m_coral2;
    CoralDetection m_coral3;
    CoralDetection m_coral4;
};

template <>
struct wpi::Struct<ObjectDetectionPacket>
{
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionPacket"; }
    static constexpr size_t GetSize() { return 4 + 4+ 8 + 4 + 12 * 4; }
    static constexpr std::string_view GetSchema()
    {
        return "int m_key;int m_versionNumber;long m_time;int m_count;CoralDetection m_coral1;CoralDetection m_coral2;CoralDetection m_coral3;CoralDetection m_coral4";
    }

    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value);
    static void ForEachNested(
        std::invocable<std::string_view, std::string_view> auto fn)
    {
        wpi::ForEachStructSchema<CoralDetection>(fn);
        wpi::ForEachStructSchema<CoralDetection>(fn);
        wpi::ForEachStructSchema<CoralDetection>(fn);
        wpi::ForEachStructSchema<CoralDetection>(fn);
    }
};

static_assert(wpi::StructSerializable<CoralDetection>);
static_assert(wpi::HasNestedStruct<ObjectDetectionPacket>);