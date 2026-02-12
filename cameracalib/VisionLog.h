#pragma once
#include <fstream>
#include <filesystem>
#include <functional>
#include <LogFrameType.h>
#include <opencv2/core.hpp>
#include <optional>

namespace fs = std::filesystem;

/** @class VisionLog
 * @version 2.0
 * @brief Base class of @ref VisionLogReader and @ref VisionLogWriter , used to fetch common properties that they both share
 * @details Holds the header data of the logfile that is currently being read / written to
 * */
class VisionLogV2 {
	friend class VisionLogReader;
	friend class VisionLogWriter;
private:
    /** @brief The width of frames in the current logfile */
	unsigned int m_resWidth;
    /** @brief The height of frames in the current logfile */
	unsigned int m_resHeight;
    /** @brief The type of frames in the current logfile (usually corresponding to channel count) */
	LogFrameType m_logType;
public:
    /** @brief Getter for @ref m_resWidth */
	unsigned int GetWidth() const;
    /** @brief Getter for @ref m_resHeight */
	unsigned int GetHeight() const;
    /** @brief Getter for @ref m_logType */
	const LogFrameType& GetLogType() const;
};

/**
 * @class VisionLogReader
 * @version 2.0
 * @brief Reads a logfile to produce cv::Mat frames as an output
 * @warning This class does NOT guarantee any thread safety
 * @warning This class will not stop you from calling ReadGrayscaleFrame when the log file is actually of type RGB or vice versa
 */
class VisionLogReader : public VisionLogV2
{
private:
    /** @brief File handle of the logfile being read */
	std::ifstream m_logfile;
    /** @brief Read the next frame as a grayscale frame and automatically move the read location forward
     * @returns a cv::Mat with @ref m_resWidth cols and @ref m_resHeight rows with CV_8UC1 channel spec
     * @returns a null optional when reading the next frame fails, usually because there is no next frame to read
     */
	std::optional<cv::Mat> TryReadGrayscaleFrame();
    /** @brief Read the next frame as a RGB frame and automatically move the read location forward
     * @returns a cv::Mat with @ref m_resWidth cols and @ref m_resHeight rows with CV_8UC3 channel spec
     * @returns a null optional when reading the next frame fails, usually because there is no next frame to read
     */
	std::optional<cv::Mat> TryReadRGBFrame();

    /** @brief The current read function, set in the constructor depending on the header data of the logfile
     * @details Set to a lambda of @ref ReadGrayscaleFrame if @ref LogFrameType::GRAYSCALE, or @ref ReadRGBFrame if @ref LogFrameType::RGB
     */
    std::function<std::optional<cv::Mat>()> m_readFrameFun;
    /** @brief Size of each frame in bytes */
	int m_frameSize;
public:
    /** @brief Create a new @ref VisionLogReader from the path to the logfile
     * @param logPath path to the logfile
     */
	VisionLogReader(fs::path logPath);

    /** @brief Read the next frame
     * @returns nullptr if there is no next frame to read or it cannot be read
     * @returns A cv::Mat pointer corresponding to the return types of @ref ReadGrayscaleFrame or @ref ReadRGBFrame depending on the log frame type
     */
	cv::Mat ReadFrame();
    /** @brief Skip to a certain frame number
     * @param frameNum the 0 indexed frame number to jump to, can be either ahead of or behind the current position. Must not be negative
     */
    std::optional<cv::Mat> TryReadFrame();

	void SkipToFrame(int frameNum);
};

/**
 * @class VisionLogWriter
 * @version 2.0
 * @brief A class for synchronously writing to a vision logfile
 * @warning This class is not thread safe in any manner
 * @warning This class will likely cause crashes if you provide invalid cv::Mat instances to @ref WriteFrame
 * @details This class follows RAII principles, logfile is closed when the writer instance is deleted
 */
class VisionLogWriter : public VisionLogV2 {
private:
    /** @brief File handle to the logfile being written to */
	std::ofstream m_logfile;
    /** @brief The size of each frame in bytes */
	int m_frameSize;
public:
    /**
     * @brief Construct a @ref VisionLogWriter from the data needed for the header
     * @param logPath the path the logfile will be written to, will overwrite if one is already present
     * @param frameType the type (usually corresponds to channel count) of the frame to write
     * @param resWidth The width of the frames to write
     * @param resHeight the height of the frames to write
     */
	VisionLogWriter(fs::path logPath, LogFrameType frameType, unsigned int resWidth, unsigned int resHeight);

    /**
     * @brief Writes another frame to the log file
     * @param frame The frame to write, must have nRows of @ref m_resHeight and nCols of @ref m_resWidth, and the type corresponding to the @ref m_logType
     * @warning Likely will cause a segfault if cv::Mat is not of the expected type as detailed above
     */
	void WriteFrame(const cv::Mat& frame);
};