#include <iostream>
#include "ObjectDetectionStruct.h"
#include "ObjectDetectionStruct.cpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <asio.hpp> // Include asio

#include "config.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "27021"
#define PI_IP "192.168.1.173"
#define OLEG_IP "192.168.1.142"
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

using asio::ip::udp;

// Function to send the ObjectDetectionPacket over to the device
void sendObjectDetectionPacket(const ObjectDetectionPacket& packet, const std::string& host, const std::string& port) {
    try {
        //Setting up I/O Object to handle async opperations while sending data
        asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint receiver_endpoint = *resolver.resolve(udp::v4(), host, port).begin();
        
        //Setting up the ability to send the packet over to the device 
        udp::socket socket(io_context);
        socket.open(udp::v4());

        // Serialize the packet and send it over the network
        std::vector<uint8_t> send_buf(ObjectDetectionPacketStructType::GetSize()); // Use the correct size!
        std::span<uint8_t> packetBytes(send_buf.data(), send_buf.size());
        ObjectDetectionPacketStructType::Pack(packetBytes, packet);

        socket.send_to(asio::buffer(send_buf), receiver_endpoint);
        #ifdef Debug_Flag
        std::cout << "Packet sent to " << host << ":" << port << std::endl;
        #endif
    //ensuring that if the packet doesn't send correctly the code will not crash
    } catch (std::exception& e) {
        #ifdef Debug_Flag
        std::cerr << "Exception in sendObjectDetectionPacket: " << e.what() << std::endl;
        #endif
    }
}

int main( int argc, char **argv ){
    #ifdef Debug_Flag
    std::cout << "test123\n"; 

    ObjectDetectionInfo coral1 = ObjectDetectionInfo(25.2f, 51.6f, 50, 1); //TODO Merge with object detection to get non-test data
    ObjectDetectionInfo coral2 = ObjectDetectionInfo(50.2f, 24.3f, 15, 1);

    std::cout << "initialized detects successfully\n"; //TODO Add debug flag to disable

    //TODO add debug flag for test seralization and deserialization (from line below to next comment)
    std::vector<uint8_t> coralBytesVec(1000); 
    std::span<uint8_t> coralBytes(coralBytesVec.data(), coralBytesVec.size());
    StructTypeObjectDetectionInfo::Pack(coralBytes, coral1);
    ObjectDetectionInfo unpackedCoral = StructTypeObjectDetectionInfo::Unpack(coralBytes);
    std::cout << "unpackedCoral dist: " << unpackedCoral.m_distance << "\n";
    std::cout << "unpackedCoral angle: " << unpackedCoral.m_angle << "\n";
    std::cout << "unpackedCoral quality: " << unpackedCoral.m_quality << "\n";
    std::cout << "unpackedCoral type: " << unpackedCoral.m_type << "\n";
    if(unpackedCoral.m_distance == coral1.m_distance && unpackedCoral.m_angle == coral1.m_angle && unpackedCoral.m_quality == coral1.m_quality && unpackedCoral.m_type == coral1.m_type){
        std::cout << ":D coral serialized and deserialized successfully" << "\n";
    }
    else{
        std::cout << "coral is sad :C" << "\n";
    } 

    //Serializing is not needed here because it is already done in ObjectDetectionPacket, and is only used for debugging
    std::vector<uint8_t> packetBytesVec(1000);
    std::span<uint8_t> packetBytes(packetBytesVec.data(), packetBytesVec.size());
    ObjectDetectionPacket packet(5, 1, 123123L, 2, coral1, coral2, ObjectDetectionInfo(), ObjectDetectionInfo());
    StructTypeObjectDetectionPacket::Pack(packetBytes, packet);
    ObjectDetectionPacket unpackedPacket = StructTypeObjectDetectionPacket::Unpack(packetBytes);
    //End of debugging flag for serialization

    //TODO Debug flag for all values we're printing simply to check if we get the right ones
    std::cout << "unpacked properties:" << "\n"; 
    std::cout << "key: " << unpackedPacket.m_key << "\n";
    std::cout << "version: " << unpackedPacket.m_versionNumber << "\n";
    std::cout << "time: " << unpackedPacket.m_time << "\n";
    std::cout << "count: " << unpackedPacket.m_count << "\n";
    std::cout << "distance1: " << unpackedPacket.m_coral1.m_distance << "\n";
    std::cout << "angle1: " << unpackedPacket.m_coral1.m_angle << "\n";
    std::cout << "quality1: " << unpackedPacket.m_coral1.m_quality << "\n";
    std::cout << "type1: " << unpackedPacket.m_coral1.m_type << "\n";
    std::cout << "distance2: " << unpackedPacket.m_coral2.m_distance << "\n";
    std::cout << "angle2: " << unpackedPacket.m_coral2.m_angle << "\n";
    std::cout << "quality2: " << unpackedPacket.m_coral2.m_quality << "\n";
    std::cout << "type2: " << unpackedPacket.m_coral2.m_type << "\n";
    std::cout << "distance: " << unpackedPacket.m_coral3.m_distance << "\n";
    std::cout << "angle3: " << unpackedPacket.m_coral3.m_angle << "\n";
    std::cout << "quality3: " << unpackedPacket.m_coral3.m_quality << "\n";
    std::cout << "type3: " << unpackedPacket.m_coral3.m_type << "\n";
    std::cout << "distance4: " << unpackedPacket.m_coral4.m_distance << "\n";
    std::cout << "angle4: " << unpackedPacket.m_coral4.m_angle << "\n";
    std::cout << "quality4: " << unpackedPacket.m_coral4.m_quality << "\n";
    std::cout << "type4: " << unpackedPacket.m_coral4.m_type << "\n";
    //to here

    //TODO Debug flag this code so we can check if the packet test worked
    if(unpackedPacket.m_key == packet.m_key && unpackedPacket.m_count == packet.m_count && unpackedPacket.m_time == packet.m_time && unpackedPacket.m_versionNumber == packet.m_versionNumber && unpackedPacket.m_coral1.m_distance == packet.m_coral1.m_distance && unpackedPacket.m_coral1.m_angle == packet.m_coral1.m_angle && unpackedPacket.m_coral1.m_quality == packet.m_coral1.m_quality && unpackedPacket.m_coral2.m_distance == packet.m_coral2.m_distance && unpackedPacket.m_coral2.m_angle == packet.m_coral2.m_angle && unpackedPacket.m_coral2.m_quality == packet.m_coral2.m_quality && unpackedPacket.m_coral3.m_distance == packet.m_coral3.m_distance && unpackedPacket.m_coral3.m_angle == packet.m_coral3.m_angle && unpackedPacket.m_coral3.m_quality == packet.m_coral3.m_quality && unpackedPacket.m_coral4.m_distance == packet.m_coral4.m_distance && unpackedPacket.m_coral4.m_angle == packet.m_coral4.m_angle && unpackedPacket.m_coral4.m_quality == packet.m_coral4.m_quality && unpackedPacket.m_coral1.m_type == packet.m_coral1.m_type && unpackedPacket.m_coral2.m_type == packet.m_coral2.m_type && unpackedPacket.m_coral3.m_type == packet.m_coral3.m_type && unpackedPacket.m_coral4.m_type == packet.m_coral4.m_type){
        std::cout << "packet serialized and deserialized successfully" << "\n"; /
    }
    else{
        std::cout << "packet test failed :C" << "\n";
    }
    //Debug flag ends here
    #endif

    sendObjectDetectionPacket(packet, OLEG_IP, SERVER_PORT); //Mandatory - Telling the code to go send the thing
} 