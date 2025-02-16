#pragma once
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

class ObjectDetectionInfo
{
public:
    ObjectDetectionInfo();
    ObjectDetectionInfo(double distance, double angle, uint8_t quality, uint8_t type);
    uint8_t m_type;
    double m_distance;
    double m_angle;
    uint8_t m_quality;
};

template <>
struct wpi::Struct<ObjectDetectionInfo>
{
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionInfo"; }
    static constexpr size_t GetSize() { return 18; }
    static constexpr std::string_view GetSchema()
    {
        return "uint8 type;double distance;double angle;uint8 quality";
    }

    static ObjectDetectionInfo Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionInfo &value);
};

static_assert(wpi::StructSerializable<ObjectDetectionInfo>);

class ObjectDetectionPacket
{
public:
    ObjectDetectionPacket();
    ObjectDetectionPacket(int key, int versionNumber, long time, uint8_t count, ObjectDetectionInfo coral1, ObjectDetectionInfo coral2, ObjectDetectionInfo coral3, ObjectDetectionInfo coral4);
    int m_key;
    int m_versionNumber;
    long m_time;
    uint8_t m_count;
    ObjectDetectionInfo m_coral1;
    ObjectDetectionInfo m_coral2;
    ObjectDetectionInfo m_coral3;
    ObjectDetectionInfo m_coral4;
};

template <>
struct wpi::Struct<ObjectDetectionPacket>
{
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionPacket"; }
    static constexpr size_t GetSize() { return 4 + 4 + 8 + 1 + 18 * 4; }
    static constexpr std::string_view GetSchema()
    {
        return "int key;int version;long timestamp;uint8 count;ObjectDetectionInfo objects[4]";
    }

    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value);
    static void ForEachNested(
        std::invocable<std::string_view, std::string_view> auto fn)
    {
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
    }
};

static_assert(wpi::StructSerializable<ObjectDetectionInfo>);
static_assert(wpi::HasNestedStruct<ObjectDetectionPacket>);