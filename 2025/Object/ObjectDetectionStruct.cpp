/*
* \brief This file defines the serialization and deserialization functions for the UDP packet data structures found in ObjectDetectionStruct.h
* \brief The code in this file can be tested using udppacketstest.cpp
*/

#include "ObjectDetectionStruct.h"
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

namespace
{
    constexpr size_t typeOff = 0; // byte offset of type byte for \ref ObjectDetectionInfo ["ObjectDetectionInfo"]
    constexpr size_t distOff = typeOff + 1; // byte offset of distance for \ref ObjectDetectionInfo ["ObjectDetectionInfo"]
    constexpr size_t angleOff = distOff + 8; // byte offset of angle for \ref ObjectDetectionInfo ["ObjectDetectionInfo"]
    constexpr size_t qualityOff = angleOff + 8; // byte offset of quality for \ref ObjectDetectionInfo ["ObjectDetectionInfo"]
    constexpr size_t keyOff = 0; // byte offset of packet key for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t versionOff = keyOff + 4; // byte offset of version number for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t timeOff = versionOff + 4; // byte offset of microseconds timestamp for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t countOff = timeOff + 8; // byte offset of detections count for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t coral1Off = countOff + 1; // byte offset of coral1 for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t coral2Off = coral1Off + 18; // byte offset of coral2 for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t coral3Off = coral2Off + 18; // byte offset of coral3 key for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
    constexpr size_t coral4Off = coral3Off + 18; // byte offset of coral4 key for \ref ObjectDetectionPacket ["ObjectDetectionPacket"]
} // namespace

/* \brief 
* \brief Creates an ObjectDetection from its distance from the camera, its angle relative to the camera, the quality of the detection
* \brief and the type of detection(see \ref ObjectDetectionTypes ["ObjectDetectionTypes"])
* \param  distance - The distance of the object from the camera as a distance on the ground in meters
* \param  angle - The angle of the detection relative to the camera
* \param  quality - The quality of the detection
* \param  type - Type of detection(see \ref ObjectDetectionTypes ["ObjectDetectionTypes"])
*/
ObjectDetectionInfo::ObjectDetectionInfo(double distance, double angle, uint8_t quality, uint8_t type) : m_distance(distance), m_angle(angle), m_quality(quality), m_type(type) {}

/* \brief 
* \brief Creates an empty \link ObjectDetectionInfo with all values set to 0 
*/
ObjectDetectionInfo::ObjectDetectionInfo()
{
    m_distance = 0.0f;
    m_angle = 0.0f;
    m_quality = 0;
    m_type = 0;
}

using ObjectDetectionInfoStructType = wpi::Struct<ObjectDetectionInfo>; // Importing the structure that will contain the serialization and deserialization functions for the ObjectDetection

/* \brief 
* \brief Deserializes an ObjectDetectionInfo from a span of bytes. 
* \brief Returns the deserialized class(\link ObjectDetectionInfo ["ObjectDetectionInfo"]) 
*/
ObjectDetectionInfo ObjectDetectionInfoStructType::Unpack(std::span<const uint8_t> data)
{
    // Unpack the data into the span using the type and the offsets found at the top of this file
    uint8_t type = wpi::UnpackStruct<uint8_t, typeOff>(data);
    double dist = wpi::UnpackStruct<double, distOff>(data);
    double angle = wpi::UnpackStruct<double, angleOff>(data);
    uint8_t quality = wpi::UnpackStruct<int, qualityOff>(data);
    return ObjectDetectionInfo(dist, angle, quality, type);
}

/* \brief Serializes an ObjectDetectionInfo into a span of bytes to be sent as a UDP packet
* \param  data - a bytes span, should be large enough to hold the serialized data(see \link ObjectDetectionInfoStructType::GetSize() ["GetSize()"])
* \param  value - The ObjectDetectionInfo class to serialize */
void ObjectDetectionInfoStructType::Pack(std::span<uint8_t> data, const ObjectDetectionInfo &value)
{
    // Pack the data into the span using the offsets found at the top of this file, detects the type automatically from the inputted value
    wpi::PackStruct<typeOff>(data, value.m_type);
    wpi::PackStruct<distOff>(data, value.m_distance);
    wpi::PackStruct<angleOff>(data, value.m_angle);
    wpi::PackStruct<qualityOff>(data, value.m_quality);
}

/* \brief 
* \brief Initializes an empty ObjectDetectionPacket 
*/
ObjectDetectionPacket::ObjectDetectionPacket() ["ObjectDetectionPacket()"]
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

/* \brief 
* \brief Initializes a complete ObjectDetectionPacket. 
* \param  key - Magic number used as a packet corruption measure, use \link PACKET_KEY to set
* \param  versionNumber - Version number used to prevent reading packets from wrong serialization version, use \link PACKET_VERSION to set this
* \param  timestamp - Server-adjusted timestamp obtained from NetworkTables in microseconds
* \param  count - How many of the corals have valid data in them, i.e 2 means coral1 and coral2 have data
* \param  coral1 - The first coral, can use \link ObjectDetectionPacket() ["ObjectDetectionPacket()"] to initialize empty
* \param  coral2 - The second coral, can use \link ObjectDetectionPacket() ["ObjectDetectionPacket()"] to initialize empty
* \param  coral3 - The third coral, can use \link ObjectDetectionPacket() ["ObjectDetectionPacket()"] to initialize empty
* \param  coral4 - The fourth coral, can use \link ObjectDetectionPacket() ["ObjectDetectionPacket()"] to initialize empty 
*/
ObjectDetectionPacket::ObjectDetectionPacket(int key, int versionNumber, long timestamp, uint8_t count, ObjectDetectionInfo coral1, ObjectDetectionInfo coral2, ObjectDetectionInfo coral3, ObjectDetectionInfo coral4) 
: m_key(key), m_versionNumber(versionNumber), m_time(timestamp), m_count(count), m_coral1(coral1), m_coral2(coral2), m_coral3(coral3), m_coral4(coral4) {}

using ObjectDetectionPacketStructType = wpi::Struct<ObjectDetectionPacket>; // Structure class that will contain the serialization and deserialization functions for the object detection packet

/* \brief 
* \brief Deserializes an ObjectDetectionPacket from a byte span. Returns the data in a \link ObjectDetectionPacket ["ObjectDetectionPacket"] class instance. 
*/
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

/* \brief Serializes an ObjectDetectionPacket into the provided byte span
* \param  data - Byte span that has enough size to hold the ObjectDetectionPacket(see \link ObjectDetectionPacketStructType::GetSize() ["GetSize()"])
* \param  value - ObjectDetectionPacket to be serialized 
*/
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