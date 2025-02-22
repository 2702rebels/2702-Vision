/*
* \brief Defines the structure of the UDP object detection packets that will be sent to the roborio. The serialization and deserialization
* \brief functions are location in ObjectDetectionStruct.cpp. The code in this file can be tested by using udppacketstest.cpp
*/

#pragma once
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

#define PACKET_KEY 5
#define PACKET_VERSION 1

/* \brief 
* \brief An enum that defines what the type byte means in the object detection schema 
*/
enum ObjectDetectionTypes {
    CORAL
};

/* \brief 
* \brief Class correlating to ObjectDetectionInfo structure, provides an accessible way to access the deserialized data and pass in data for serialization 
*/
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
/* \brief 
* \brief Structure defining how an object detection should be serialized 
*/
struct wpi::Struct<ObjectDetectionInfo>
{
    /* \brief 
    * \brief Returns the name of the class that represents the data in the packet as a string 
    */
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionInfo"; }
    /* \brief 
    * \brief Returns the size of the packet in bytes. This is a constant value 
    */
    static constexpr size_t GetSize() { return 18; }
    /* \brief 
    * \brief Returns the schema detailing the format of the packet as a string 
    */
    static constexpr std::string_view GetSchema()
    {
        return "uint8 type;double distance;double angle;uint8 quality";
    }

    static ObjectDetectionInfo Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionInfo &value);
};

static_assert(wpi::StructSerializable<ObjectDetectionInfo>); // Ensure that the ObjectDetection is serializable

/* \brief 
* \brief Class correlating to the ObjectDetectionPacket structure used for UDP packet serialization / deserialization. 
* \brief Provides an accessible way to access the deserialized data and pass in data for serialization. 
*/
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
/* \brief 
* \brief Structure defining how an ObjectDetectionPacket should be serialized. 
*/
struct wpi::Struct<ObjectDetectionPacket>
{
    /* \brief 
    * \brief Returns the name of the class that represents the data in this packet as a string 
    */
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionPacket"; }
    /* \brief 
    * \brief Returns the size of the packet in bytes. This is a constant value. 
    */
    static constexpr size_t GetSize() { return 4 + 4 + 8 + 1 + 18 * \brief 4; }
    /* \brief 
    * \brief Returns the schema of the packet detailing its format as a string 
    */
    static constexpr std::string_view GetSchema()
    {
        return "int key;int version;long timestamp;uint8 count;ObjectDetectionInfo objects[4]";
    }

    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value);
    /* \brief 
    * \brief WPIlib thing, if any additional classes are added to the packet, please list them here 
    */
    static void ForEachNested(
        std::invocable<std::string_view, std::string_view> auto fn)
    {
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
        wpi::ForEachStructSchema<ObjectDetectionInfo>(fn);
    }
};

static_assert(wpi::StructSerializable<ObjectDetectionPacket>); // Checks that the object detection packet is serializable
static_assert(wpi::HasNestedStruct<ObjectDetectionPacket>);