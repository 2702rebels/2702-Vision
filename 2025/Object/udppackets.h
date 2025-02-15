#include <string>
#include <vector>

#define PACKET_KEY "DEADBEEF"
#define PACKET_VERSION_NUMBER 1
#define CORAL_DETECTION_SIZE_BYTES 12

class CoralDetection {
    public:
    CoralDetection(float distance, float angle, long time);
    CoralDetection(std::vector<uint8_t> bytes);
    std::vector<uint8_t> ToBytes();
    float m_distance;
    float m_angle;
    long m_time;
};

class ObjectDetectionPacket {
    public:
    ObjectDetectionPacket(std::vector<uint8_t> bytes);
    ObjectDetectionPacket(int detectionCount, std::vector<CoralDetection> coralDetects);
    std::vector<uint8_t> ToBytes();
    int m_count;
    std::vector<CoralDetection> m_coralDetects;
    private:
    std::string m_packetKey;
    int m_versionNumber;
};