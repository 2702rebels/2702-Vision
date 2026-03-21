#include <cstdint>

/**
 * @class LogFrameType
 * @brief The different types of frames that can be read/written by @ref
 * VisionLogReader and @ref VisionLogWriter
 * @details @ref GRAYSCALE frames come from @ref CV_8UC1 @ref cv::Mat class
 * instances.
 * @ref RGB frames come from @ref CV_8UC3 @ref cv::Mat class instances
 */
enum class LogFrameType : unsigned int { GRAYSCALE, RGB };