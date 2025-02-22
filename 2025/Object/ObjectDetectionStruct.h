/*!
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

/*!
* \brief An enum that defines what the type byte means in the object detection schema 
*/
enum ObjectDetectionTypes {
    /*! \brief Coral object detection. */
    CORAL
};

/*! \brief Class correlating to \ref wpi::Struct<ObjectDetectionInfo> structure, provides an accessible way to access the deserialized data and pass in data for serialization 
*/
class ObjectDetectionInfo
{
public:
    ObjectDetectionInfo();
    ObjectDetectionInfo(double distance, double angle, uint8_t quality, uint8_t type);
    /*! \brief Object type, corresponds to an enum value from \ref ObjectDetectionTypes . */
    uint8_t m_type;
    /*! \brief 2D plane distance of object from camera. */
    double m_distance;
    /*! \brief Angle of object relative to camera. */
    double m_angle;
    /*! \brief Quality of the detection, usually derived primarily from distance and how much the object matches the pattern. */
    uint8_t m_quality;
};

template <>
/*! \brief Structure defining how an object detection should be serialized 
*/
struct wpi::Struct<ObjectDetectionInfo>
{
    /*! \brief Returns the name of the class that represents the data in the packet as a string */
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionInfo"; }
    /*! \brief Returns the size of the packet in bytes. This is a constant value */
    static constexpr size_t GetSize() { return 18; }
    /*! \brief Returns the schema detailing the format of the packet as a string */
    static constexpr std::string_view GetSchema()
    {
        return "uint8 type;double distance;double angle;uint8 quality";
    }

    /*! \brief Deserializes an \ref ObjectDetectionInfo from a span of bytes 
    * \param data - A span of bytes containing the ObjectDetectionInfo
    * \returns The deserialized \ref ObjectDetectionInfo
    */
    static ObjectDetectionInfo Unpack(std::span<const uint8_t> data);
    /*! \brief Serializes an \ref ObjectDetectionInfo into a span of bytes
    * \param data - A span of bytes large enough to hold the \ref ObjectDetectionInfo (see \ref GetSize())
    * \param value - The \ref ObjectDetectionInfo to serialize
    */
    static void Pack(std::span<uint8_t> data, const ObjectDetectionInfo &value);
};

static_assert(wpi::StructSerializable<ObjectDetectionInfo>); // Ensure that the ObjectDetection is serializable

/*! \brief Class correlating to the \ref wpi::Struct<ObjectDetectionPacket> structure used for UDP packet serialization / deserialization. 
* Provides an accessible way to access the deserialized data and pass in data for serialization. Holds a max of 4 objects
*/
class ObjectDetectionPacket
{
public:
    ObjectDetectionPacket();
    ObjectDetectionPacket(int key, int versionNumber, long time, uint8_t count, ObjectDetectionInfo coral1, ObjectDetectionInfo coral2, ObjectDetectionInfo coral3, ObjectDetectionInfo coral4);
    /*! \brief Magic number used to check for corruption / invalid packet, set using \ref PACKET_KEY */
    int m_key;
    /*! \brief Version number of packet, set using \ref PACKET_VERSION */
    int m_versionNumber;
    /*! \brief Timestamp of packet, microseconds from NetworkTables adjusted for server */
    long m_time;
    /*! \brief How many of the corals are filled with real data */
    uint8_t m_count;
    /*! \brief First coral */
    ObjectDetectionInfo m_coral1;
    /*! \brief Second coral */
    ObjectDetectionInfo m_coral2;
    /*! \brief Third coral */
    ObjectDetectionInfo m_coral3;
    /*! \brief Fourth coral */
    ObjectDetectionInfo m_coral4;
};

template <>
/*! \brief Structure defining how an ObjectDetectionPacket should be serialized. */
struct wpi::Struct<ObjectDetectionPacket>
{
    /*! \brief  Returns the name of the class that represents the data in this packet as a string.  */
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionPacket"; }
    /*! \brief Returns the size of the packet in bytes. This is a constant value. */
    static constexpr size_t GetSize() { return 4 + 4 + 8 + 1 + 18 * \brief 4; }
    /*! \brief Returns the schema of the packet detailing its format as a string. */
    static constexpr std::string_view GetSchema()
    {
        return "int key;int version;long timestamp;uint8 count;ObjectDetectionInfo objects[4]";
    }

    /*! \brief Deserializes a \ref ObjectDetectionPacket. 
    \param data - The span of bytes containing the \ref ObjectDetectionPacket.
    \returns The \ref ObjectDetectionPacket from the data.
    */
    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    /*! \brief Serializes a \ref ObjectDetectionPacket .
    \param data - The span of bytes that will contain the \ref ObjectDetectionPacket, needs to be large enough(see \ref GetSize()).
    \param value - The \ref ObjectDetectionPacket to serialize.
    */
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value);
    /*! \brief WPIlib thing, if any additional classes are added to the packet, please list them here. */
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