#include <string>
#include <vector>
#include "udppackets.h"
#include <iostream>

CoralDetection::CoralDetection(float distance, float angle, long time) : m_distance(distance), m_angle(angle), m_time(time) {

    };

std::vector<uint8_t> CoralDetection::ToBytes()
{
    std::vector<uint8_t> byteArr(12);
    std::vector<uint8_t> distVector = std::vector<uint8_t>(reinterpret_cast<uint8_t>(&m_distance));
    std::vector<uint8_t> angleVector = std::vector<uint8_t>(reinterpret_cast<uint8_t>(&m_angle));
    std::vector<uint8_t> timeVector(4);
    timeVector.push_back((int)((m_time >> 24) & 0xFF));
    timeVector.push_back((int)((m_time >> 16) & 0xFF));
    timeVector.push_back((int)((m_time >> 8) & 0XFF));
    timeVector.push_back((int)((m_time & 0XFF)));
    byteArr.insert(byteArr.end(), distVector.begin(), distVector.end());
    byteArr.insert(byteArr.end(), angleVector.begin(), angleVector.end());
    byteArr.insert(byteArr.end(), timeVector.begin(), timeVector.end());
    return byteArr;
}

CoralDetection::CoralDetection(std::vector<uint8_t> bytes)
{
    std::vector<uint8_t> distanceVec(bytes.begin(), bytes.begin() + 3);
    std::vector<uint8_t> angleVec(bytes.begin() + 4, bytes.begin() + 7);
    std::vector<uint8_t> timeVec(bytes.begin() + 8, bytes.begin() + 11);
    memcpy(&m_distance, distanceVec.data(), sizeof(distanceVec.data()));
    memcpy(&m_angle, angleVec.data(), sizeof(angleVec.data()));
    memcpy(&m_time, timeVec.data(), sizeof(timeVec.data()));
}

ObjectDetectionPacket::ObjectDetectionPacket(int detectionCount, std::vector<CoralDetection> coralDetects) : m_count(detectionCount),
                                                                                                             m_coralDetects(coralDetects), m_packetKey(PACKET_KEY), m_versionNumber(PACKET_VERSION_NUMBER)
{
}

ObjectDetectionPacket::ObjectDetectionPacket(std::vector<uint8_t> bytes)
{
    std::vector<uint8_t> keyVec(bytes.begin(), bytes.begin() + 9);
    if (std::string(reinterpret_cast<char *>(keyVec.data())) != PACKET_KEY)
    {
        std::cout << "Incorrect packet key\n";
        return;
    }
    std::vector<uint8_t> versionVec(bytes.begin() + 10, bytes.begin() + 10);
    if (*keyVec.data() != PACKET_VERSION_NUMBER)
    {
        std::cout << "Incorrect packet version number\n";
        return;
    }
    std::vector<uint8_t> countVec(bytes.begin() + 11, bytes.begin() + 11);
    m_count = (int)countVec.data();
    for (int i = 0; i < (bytes.size() - 11) / CORAL_DETECTION_SIZE_BYTES; i++)
    {
        std::vector<uint8_t> coralDetectionVec(bytes.begin() + 12 + 12 * i, bytes.begin() + 12 + 12 * (i + 1));
        m_coralDetects.push_back(CoralDetection(coralDetectionVec));
    }
}

std::vector<uint8_t> ObjectDetectionPacket::ToBytes()
{
    std::vector<uint8_t> output;
    std::vector<uint8_t> packetKeyVec(m_packetKey.begin(), m_packetKey.end());
    output.insert(output.end(), packetKeyVec.begin(), packetKeyVec.end());
    output.push_back(m_versionNumber);
    output.push_back(m_count);
    for (int i = 0; i < m_coralDetects.size(); i++)
    {
        std::vector<uint8_t> coralDetect = m_coralDetects[i].ToBytes();
        output.insert(output.end(), coralDetect.begin(), coralDetect.end());
    }
    return output;
}