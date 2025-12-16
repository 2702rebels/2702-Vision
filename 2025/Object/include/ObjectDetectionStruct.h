/*!
* \brief Defines the structure of the UDP object detection packets that will be sent to the roborio. The serialization and deserialization
*       functions are location in ObjectDetectionStruct.cpp. The code in this file can be tested by using udppacketstest.cpp
*/

#pragma once
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

#define PACKET_KEY 5
#define PACKET_VERSION 1

/**
 * @class ObjectDetectionTypes
 * @brief Possible values for @ref ObjectDetectionInfo::m_type
 * @version 1.0
 */
enum class ObjectDetectionTypes : uint8_t {
    CORAL
};

/**
 * @class ObjectDetectionInfo
 * @brief A class corresponding to the info about a singular object detection
 * @details A wpilib serializable struct that is turned into a UDP packet for transmission to the roboRIO where it is used to effect a change
 * @version 1.0
 */
class ObjectDetectionInfo
{
public:
    /** @brief Create an empty @ref ObjectDetectionInfo with blank values */
    ObjectDetectionInfo();
    /** @brief Create a new @ref ObjectDetectionInfo from the properties of the object detected
     * @param distance The distance of the detection from the origin of the robot
     * @param angle The angle of the object from the origin of the robot in radians (note FRC directions for radians)
     * @param quality Currently unused parameter to describe the confidence of the algorithm in the detection
     * @param type The type of object detected by the processor, must be a member of the @ref ObjectDetectionTypes enum
     */
    ObjectDetectionInfo(double distance, double angle, uint8_t quality, uint8_t type);
    /** @brief Object type, corresponds to an enum value from @ref ObjectDetectionTypes */
    uint8_t m_type;
    /** @brief The distance of the detection from the origin of the robot  */
    double m_distance;
    /** @brief The angle of the object from the origin of the robot in radians (note FRC directions for radians) */
    double m_angle;
    /** @brief Currently unused parameter to describe the confidence of the algorithm in the detection */
    uint8_t m_quality;
    /** @brief Operator to check if a detection is equal to another one by checking equality of all members
     * @bug This will likely return false even when they are the same due to floating point errors in the @ref m_angle and @ref m_distance values
     */
    constexpr bool operator==(const ObjectDetectionInfo& other) const {
        if(other.m_distance == m_distance && other.m_angle == m_angle && other.m_quality == m_quality && other.m_type == m_type){
            return true;
        }
        return false;
    }

    /* @brief Operator to enable sorting of object detections
     * @returns true if the current detection's distance is shorter than other's detection distance
     */
    constexpr bool operator<(const ObjectDetectionInfo& other) const {
        if(m_distance < other.m_distance){
            return true;
        }
        return false;
    }
};

template <>
/** @brief WPI structure info for the serialization of a @ref ObjectDetectionInfo
*/
struct wpi::Struct<ObjectDetectionInfo>
{
    /** @brief Returns the name of the class that represents the data in the packet as a string */
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionInfo"; }
    /** @brief Returns the size of the packet in bytes. This is a constant value */
    static constexpr size_t GetSize() { return 18; }
    /** @brief Returns the schema detailing the format of the packet as a string in protobuf format */
    static constexpr std::string_view GetSchema()
    {
        return "uint8 type;double distance;double angle;uint8 quality";
    }

    /** @brief Deserializes an @ref ObjectDetectionInfo from a span of bytes 
    * @param data - A span of bytes containing the @ref ObjectDetectionInfo
    * @returns The deserialized @ref ObjectDetectionInfo
    */
    static ObjectDetectionInfo Unpack(std::span<const uint8_t> data);
    /** @brief Serializes an @ref ObjectDetectionInfo into a span of bytes
    * @param data - A span of bytes large enough to hold the @ref ObjectDetectionInfo (see @ref GetSize())
    * @param value - The @ref ObjectDetectionInfo to serialize
    */
    static void Pack(std::span<uint8_t> data, const ObjectDetectionInfo &value);
};

static_assert(wpi::StructSerializable<ObjectDetectionInfo>); // Ensure that the ObjectDetection is serializable

/**
 * @class ObjectDetectionPacket
 * @brief Packet holding multiple object detections and other header info
 * @details Also has an associated WPI struct for serialization and transmission to the roboRIO by UDP packet
 * @version 1.0
*/
class ObjectDetectionPacket
{
public:
    /** @brief Create a new empty @ref ObjectDetectionPacket */
    ObjectDetectionPacket();
    /** @brief Create a new @ref ObjectDetectionPacket from all fields
     * @param key A magic value used to check the integrity of the packet, defined as @ref PACKET_KEY
     * @param versionNumber Version number used to check the validity of the packet as well, defined as @ref PACKET_VERSION
     * @param time Timestamp of packet, microseconds from NetworkTables adjusted for server
     * @param count The number of valid object detections in this packet
     * @param coral1 The first detection
     * @param coral2 The second detection
     * @param coral3 The third detetion
     * @param coral4 The fourth detection
     */
    ObjectDetectionPacket(int key, int versionNumber, long time, uint8_t count, ObjectDetectionInfo coral1, ObjectDetectionInfo coral2, ObjectDetectionInfo coral3, ObjectDetectionInfo coral4);
    /** @brief Magic number used to check for corruption or invalid packet, set using @ref PACKET_KEY */
    int m_key;
    /** @brief Version number of packet, set using @ref PACKET_VERSION */
    int m_versionNumber;
    /** @brief Timestamp of packet, microseconds from NetworkTables adjusted for server */
    long m_time;
    /** @brief How many of the corals are filled with real data */
    uint8_t m_count;
    /** @brief First coral */
    ObjectDetectionInfo m_coral1;
    /** @brief Second coral */
    ObjectDetectionInfo m_coral2;
    /** @brief Third coral */
    ObjectDetectionInfo m_coral3;
    /** @brief Fourth coral */
    ObjectDetectionInfo m_coral4;
    /** @brief Operation for comparing if an @ref ObjectDetectionPacket is exactly equal to another
     * @bug Likely to fail even when they are the same due to error in floating point comparisons in @ref ObjectDetectionInfo equals operator
     */
    constexpr bool operator==(const ObjectDetectionPacket& other) const {
        if(other.m_key == m_key && other.m_count == m_count && other.m_time == m_time && other.m_versionNumber == m_versionNumber && other.m_coral1 == m_coral1 && other.m_coral2 == m_coral2 && other.m_coral3 == m_coral3 && other.m_coral4 == m_coral4){
            return true;
        }
        return false;
    }
};

template <>
/** @brief Structure defining how an @ref ObjectDetectionPacket should be serialized. */
struct wpi::Struct<ObjectDetectionPacket>
{
    /** @brief  Returns the name of the class that represents the data in this packet as a string.  */
    static constexpr std::string_view GetTypeName() { return "ObjectDetectionPacket"; }
    /** @brief Returns the size of the packet in bytes. This is a constant value. */
    static constexpr size_t GetSize() { return 4 + 4 + 8 + 1 + 18 * 4; }
    /** @brief Returns the schema of the packet detailing its format as a string. */
    static constexpr std::string_view GetSchema()
    {
        return "int key;int version;long timestamp;uint8 count;ObjectDetectionInfo objects[4]";
    }

    /** @brief Deserializes a @ref ObjectDetectionPacket. 
    \param data - The span of bytes containing the @ref ObjectDetectionPacket.
    \returns The @ref ObjectDetectionPacket from the data.
    */
    static ObjectDetectionPacket Unpack(std::span<const uint8_t> data);
    /** @brief Serializes a @ref ObjectDetectionPacket .
    \param data - The span of bytes that will contain the @ref ObjectDetectionPacket, needs to be large enough(see @ref GetSize()).
    \param value - The @ref ObjectDetectionPacket to serialize.
    */
    static void Pack(std::span<uint8_t> data, const ObjectDetectionPacket &value);
    /** @brief WPIlib thing, if any additional classes are added to the packet, please list them here. */
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