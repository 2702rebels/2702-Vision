#include <gtest/gtest.h>
#include "../2025/Object/ObjectDetectionStruct.h"

namespace {
    TEST(SerializationTest, ObjectDetectionInfoSerialize) {
        ObjectDetectionInfo coral1 = ObjectDetectionInfo(25.2f, 51.6f, 50, 0);
        std::vector<uint8_t> coralBytesVec(1000);
        std::span<uint8_t> coralBytes(coralBytesVec.data(), coralBytesVec.size());
        StructTypeObjectDetectionInfo::Pack(coralBytes, coral1);
        ObjectDetectionInfo unpackedCoral = StructTypeObjectDetectionInfo::Unpack(coralBytes);
        EXPECT_EQ(coral1, unpackedCoral);
    }
    
    TEST(SerializationTest, ObjectDetectionPacketSerialize) {
        // This will also fail if ObjectDetectionInfo serialize fails
        ObjectDetectionInfo coral1 = ObjectDetectionInfo(25.2f, 51.6f, 50, 0);
        ObjectDetectionInfo coral2 = ObjectDetectionInfo(50.2f, 24.3f, 15, 0);
        std::vector<uint8_t> packetBytesVec(1000);
        std::span<uint8_t> packetBytes(packetBytesVec.data(), packetBytesVec.size());
        // Create a testing packet
        ObjectDetectionPacket packet(5, 1, 123123L, 2, coral1, coral2, ObjectDetectionInfo(), ObjectDetectionInfo());
        StructTypeObjectDetectionPacket::Pack(packetBytes, packet);
        ObjectDetectionPacket unpackedPacket = StructTypeObjectDetectionPacket::Unpack(packetBytes);
        EXPECT_EQ(packet, unpackedPacket);
    }
}