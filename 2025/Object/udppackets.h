#include <string>
#include <vector>

#define PACKET_KEY "DEADBEEF"
#define PACKET_KEY_SIZE 9
#define PACKET_VERSION_NUMBER 1
#define CORAL_DETECTION_SIZE_BYTES 12

struct CoralDetection {
    float m_distance;
    float m_angle;
    long m_time;
};

struct ObjectDetectionPacket {
    int m_count;
    std::vector<CoralDetection> m_coralDetects;
    std::string m_packetKey;
    int m_versionNumber;
};