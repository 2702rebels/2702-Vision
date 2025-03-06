/*!
* \brief This file defines the serialization and deserialization functions for the UDP packet data structures found in ObjectDetectionStruct.h
* The code in this file can be tested using udppacketstest.cpp
*/

#include "ObjectDetectionStruct.h"
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

namespace
{
    /*! \brief byte offset of mask byte */
    constexpr size_t offMsk = 0;
     /*! \brief byte offset of network timestamp */
    constexpr size_t offNetTime = offMsk + 1;
     /*! \brief byte offset of Field X pos */
    constexpr size_t offFieldX = offNetTime + 8;
    /*! \brief byte offset of Field Y pos */
    constexpr size_t offFieldY = offFieldX + 8;
	/*! \brief byte offset of Field Rot */
	constexpr size_t offFieldR = offFieldY + 8;
	/*! \brief byte offset of Quality value */
	constexpr size_t offFieldQ = offFieldR + 8;
	/*! \brief byte offset of TagCount */
	constexpr size_t offTagCnt = offFieldQ + 8;
	/*! \brief byte offset of Tags data */
	constexpr size_t offTagData = offTagCnt + 1;

	/*! \brief byte offset of Tag Id */
	constexpr size_t offTagId = 0;
	/*! \brief byte offset of Tag X */
	constexpr size_t offTagX = offTagId + 1;
	/*! \brief byte offset of Tag Y */
	constexpr size_t offTagY = offTagX + 8;
	/*! \brief byte Total size of Tag data */
	constexpr size_t offTagMax = offTaxY + 8;
}

ApriltagData::ApriltagData()
	m_u8Id( 0 ),
	m_dX( 0. ),
	m_dY( 0. )
{
}

ApriltagData::ApriltagData( uint8_t u8Id, double dX, double dY ) :
	m_u8Id( u8Id ),
	m_dX( dX ),
	m_dY( dY )
{
}

using ApriltagDataStructType = wpi::Struct<ApriltagData>; // Importing the structure that will contain the serialization and deserialization functions for the ApriltagData

ApriltagData ApriltagDataStructType::Unpack(std::span<const uint8_t> data)
{
    // Unpack the data into the span using the type and the offsets found at the top of this file
    uint8_t u8Id = wpi::UnpackStruct<uint8_t, offTagId>(data);
    double dX = wpi::UnpackStruct<double, offTagX>(data);
    double dY = wpi::UnpackStruct<double, offTagY>(data);
    return ApriltagData( u8Id, dX, dY );
}

void ApriltagDataStructType::Pack(std::span<uint8_t> data, const ApriltagData &value)
{
    // Pack the data into the span using the offsets found at the top of this file, detects the type automatically from the inputted value
    wpi::PackStruct<typeOff>(data, value.m_u8id);
    wpi::PackStruct<distOff>(data, value.m_dX);
    wpi::PackStruct<angleOff>(data, value.m_dY);
}

/*!
* \brief Initializes an empty ObjectDetectionPacket 
*/
ApriltagReport::ApriltagReport( uint64_t u64NetTime ) :
	m_u8Mask( 0 ),
	m_u64NetTime( 0 ),
	m_dX( 0. ),
	m_dY( 0. ),
	m_dTheta( 0. ),
	m_dQuality( 0. ),
	m_u8Count( 0 )
{
}

ApriltagReport::ApriltagReport( uint64_t u64NetTime, double dX, double dY, double dTheta, double dQuality ) :
	m_u8Mask( 1 ),
	m_u64NetTime( u8NetTime ),
	m_dX( dX ),
	m_dY( dY ),
	m_dTheta( dTheta ),
	m_dQuality( dQuality )
	m_u8Count( 0 )
{
}

using ApriltagReportStructType = wpi::Struct<ApriltagReport>; // Structure class that will contain the serialization and deserialization functions for the object detection packet

/*! \brief Deserializes an ObjectDetectionPacket from a byte span. Returns the data in a \ref ObjectDetectionPacket class instance. 
*/
ApriltagReport ApriltagReportStructType::Unpack(std::span<const uint8_t> data)
{
    int key = wpi::UnpackStruct<int, keyOff>(data);
    int versionNumber = wpi::UnpackStruct<int, versionOff>(data);
    long time = wpi::UnpackStruct<long, timeOff>(data);
    uint8_t count = wpi::UnpackStruct<uint8_t, countOff>(data);
    ApriltagData ad1 = wpi::UnpackStruct<ObjectDetectionInfo, coral1Off>(data);
    APriltagData ad2 = wpi::UnpackStruct<ObjectDetectionInfo, coral2Off>(data);
    APriltagData ad3 = wpi::UnpackStruct<ObjectDetectionInfo, coral3Off>(data);
    APriltagData ad4 = wpi::UnpackStruct<ObjectDetectionInfo, coral4Off>(data);
    APriltagData ad4 = wpi::UnpackStruct<ObjectDetectionInfo, coral4Off>(data);
    APriltagData ad4 = wpi::UnpackStruct<ObjectDetectionInfo, coral4Off>(data);
    return ObjectDetectionPacket(key, versionNumber, time, count, coral1, coral2, coral3, coral4);
}

/*! \brief Serializes an ObjectDetectionPacket into the provided byte span.
* \param data - Byte span that must be large enough to hold the ObjectDetectionPacket(see \ref ObjectDetectionPacketStructType::GetSize() ).
* \param value - ObjectDetectionPacket to be serialized.
*/
void ApriltagReportStructType::Pack(std::span<uint8_t> data, const ApriltagReport &value)
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
