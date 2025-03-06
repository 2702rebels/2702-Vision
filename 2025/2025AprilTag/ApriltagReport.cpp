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


class ApriltagData
{
	uint8_t		m_u8Id;
	double		m_dX;
	double		m_dY;
public:
	ApriltagData( uint8_t u8Id, double dX, double dY );
	ApriltagData( ApriltagData const &src );
	~ApriltagData();

	ApriltagData &operator=( ApriltagData const &src );
};

class ApriltagReport
{
	uint8_t		m_u8Mask;
	uint64_t	m_u64NetTime;
	double		m_dX;
	double		m_dY;
	double		m_dTheta;
	double		m_dQuality;
	
	uint8_t		m_u8Count;

	ApriltagData	m_ad1;
	ApriltagData	m_ad2;
	ApriltagData	m_ad3;
	ApriltagData	m_ad4;
	ApriltagData	m_ad5;
	ApriltagData	m_ad6;
public:
	ApriltagReport();
	ApriltagReport( double dX, double dY, double dQuality );
	~ApriltagReport();

	void vAddData( ApriltagData const &ad );
};


template <>
/*! \brief Structure defining how an object detection should be serialized 
*/
struct wpi::Struct<ApriltagData>
{
    /*! \brief Returns the name of the class that represents the data in the packet as a string */
    static constexpr std::string_view GetTypeName() { return "ApriltagData"; }
    /*! \brief Returns the size of the packet in bytes. This is a constant value */
    static constexpr size_t GetSize() { return 17; }
    /*! \brief Returns the schema detailing the format of the packet as a string */
    static constexpr std::string_view GetSchema()
    {
        return "uint8 id;double x;double y";
    }

    static ApriltagData Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ApriltagData &value);
};

static_assert(wpi::StructSerializable<ApriltagData>); // Ensure that the ObjectDetection is serializable

template <>
/*! \brief Structure defining how an ObjectDetectionPacket should be serialized. */
struct wpi::Struct<ApriltagReport>
{
    /*! \brief  Returns the name of the class that represents the data in this packet as a string.  */
    static constexpr std::string_view GetTypeName() { return "ApriltagReport"; }
    /*! \brief Returns the size of the packet in bytes. This is a constant value. */
    static constexpr size_t GetSize() { return 1 + 8 + 8 * 4 + 1 + 17 * 6; }
    /*! \brief Returns the schema of the packet detailing its format as a string. */
    static constexpr std::string_view GetSchema()
    {
        return "int mask;long timestamp;double X;double Y;double theta;double qualilty;uint8 count;ApriltagData objects[6]";
    }

    static ApriltagReport Unpack(std::span<const uint8_t> data);
    static void Pack(std::span<uint8_t> data, const ApriltagReport &value);
    /*! \brief WPIlib thing, if any additional classes are added to the packet, please list them here. */
    static void ForEachNested(
        std::invocable<std::string_view, std::string_view> auto fn)
    {
        wpi::ForEachStructSchema<ApriltagData>(fn);
        wpi::ForEachStructSchema<ApriltagData>(fn);
        wpi::ForEachStructSchema<ApriltagData>(fn);
        wpi::ForEachStructSchema<ApriltagData>(fn);
    }
};

static_assert(wpi::StructSerializable<ApriltagReport>); // Checks that the object detection packet is serializable
static_assert(wpi::HasNestedStruct<ApriltagReport>);
