/*!
* \brief This file defines the serialization and deserialization functions for the UDP packet data structures found in ObjectDetectionStruct.h
* The code in this file can be tested using udppacketstest.cpp
*/

#include "ApriltagReport.h"
#include <string>
#include <span>
#include <wpi/struct/Struct.h>
#include <iostream>
#include <vector>

namespace
{
	/*! \brief byte offset of Tag Id */
	constexpr size_t offTagId = 0;
	/*! \brief byte offset of Tag X */
	constexpr size_t offTagX = offTagId + 1;
	/*! \brief byte offset of Tag Y */
	constexpr size_t offTagY = offTagX + 8;
	/*! \brief byte Total size of Tag data */
	constexpr size_t offTagMax = offTagY + 8;

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
	constexpr size_t offTag1 = offTagData;
	constexpr size_t offTag2 = offTag1 + offTagMax;
	constexpr size_t offTag3 = offTag2 + offTagMax;
	constexpr size_t offTag4 = offTag3 + offTagMax;
	constexpr size_t offTag5 = offTag4 + offTagMax;
	constexpr size_t offTag6 = offTag5 + offTagMax;
}

ApriltagData::ApriltagData() :
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

ApriltagData::~ApriltagData()
{
}

ApriltagData &ApriltagData::operator=( ApriltagData const &src )
{
	m_u8Id = src.m_u8Id;
	m_dX = src.m_dX;
	m_dY = src.m_dY;

	return *this;
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
    wpi::PackStruct<offTagId>(data, value.m_u8Id);
    wpi::PackStruct<offTagX>(data, value.m_dX);
    wpi::PackStruct<offTagY>(data, value.m_dY);
}

ApriltagReport::ApriltagReport( uint8_t u8Mask, uint64_t u64NetTime, double dX, double dY, double dTheta, double dQuality ) :
	m_u8Mask( u8Mask ),
	m_u64NetTime( u64NetTime ),
	m_dX( dX ),
	m_dY( dY ),
	m_dTheta( dTheta ),
	m_dQuality( dQuality )
{
}

ApriltagReport::~ApriltagReport()
{
}

void ApriltagReport::vAddData( ApriltagData const &ad )
{
	switch( m_u8Tags ) {
	case 0:
		m_ad1 = ad;
		m_u8Tags++;
		break;
	case 1:
		m_ad2 = ad;
		m_u8Tags++;
		break;
	case 2:
		m_ad3 = ad;
		m_u8Tags++;
		break;
	case 3:
		m_ad4 = ad;
		m_u8Tags++;
		break;
	case 4:
		m_ad5 = ad;
		m_u8Tags++;
		break;
	case 5:
		m_ad6 = ad;
		m_u8Tags++;
		break;
	default:
		break;
	}
}

using ApriltagReportStructType = wpi::Struct<ApriltagReport>; // Structure class that will contain the serialization and deserialization functions for the object detection packet

/*! \brief Deserializes an ObjectDetectionPacket from a byte span. Returns the data in a \ref ObjectDetectionPacket class instance. 
*/
ApriltagReport ApriltagReportStructType::Unpack(std::span<const uint8_t> data)
{
	uint8_t u8Mask 		= wpi::UnpackStruct<uint8_t, offMsk>(data);
	uint64_t u64NetTime = wpi::UnpackStruct<uint64_t, offNetTime>(data);
	double dFldX		= wpi::UnpackStruct<double, offFieldX>(data);
	double dFldY		= wpi::UnpackStruct<double, offFieldY>(data);
	double dFldTheta	= wpi::UnpackStruct<double, offFieldR>(data);
	double dQuality		= wpi::UnpackStruct<double, offFieldQ>(data);
	uint8_t u8Count		= wpi::UnpackStruct<uint8_t, offTagCnt>(data);

	ApriltagReport ar( u8Mask, u64NetTime, dFldX, dFldY, dFldTheta, dQuality );

	ar.m_ad1 = wpi::UnpackStruct<ApriltagData, offTag1>(data);
	ar.m_ad2 = wpi::UnpackStruct<ApriltagData, offTag2>(data);
	ar.m_ad3 = wpi::UnpackStruct<ApriltagData, offTag3>(data);
	ar.m_ad4 = wpi::UnpackStruct<ApriltagData, offTag4>(data);
	ar.m_ad5 = wpi::UnpackStruct<ApriltagData, offTag5>(data);
	ar.m_ad6 = wpi::UnpackStruct<ApriltagData, offTag6>(data);

	return ar;
}

/*! \brief Serializes an ObjectDetectionPacket into the provided byte span.
* \param data - Byte span that must be large enough to hold the ObjectDetectionPacket(see \ref ObjectDetectionPacketStructType::GetSize() ).
* \param value - ObjectDetectionPacket to be serialized.
*/
void ApriltagReportStructType::Pack(std::span<uint8_t> data, const ApriltagReport &value)
{
	wpi::PackStruct<offMsk>(data, value.m_u8Mask);
	wpi::PackStruct<offNetTime>(data, value.m_u64NetTime);
	wpi::PackStruct<offFieldX>(data, value.m_dX);
	wpi::PackStruct<offFieldY>(data, value.m_dY);
	wpi::PackStruct<offFieldR>(data, value.m_dTheta);
	wpi::PackStruct<offFieldQ>(data, value.m_dQuality);
	wpi::PackStruct<offTagCnt>(data, value.m_u8Tags);

	wpi::PackStruct<offTag1>(data, value.m_ad1);
	wpi::PackStruct<offTag2>(data, value.m_ad2);
	wpi::PackStruct<offTag3>(data, value.m_ad3);
	wpi::PackStruct<offTag4>(data, value.m_ad4);
	wpi::PackStruct<offTag5>(data, value.m_ad5);
	wpi::PackStruct<offTag6>(data, value.m_ad6);
}
