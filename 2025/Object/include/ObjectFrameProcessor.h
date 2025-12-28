#pragma once

#include <frmprocessor.h>
#include <string>
#include <array>
#include <wpi/struct/Struct.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/NetworkTableListener.h>
#include <fstream>
#include <memory>
#include <networktables/DoubleTopic.h>
#include <networktables/IntegerTopic.h>
#include <opencv2/imgproc.hpp>
#include <cstdint>
#include <ObjectDetectionStruct.h>
#include <udpsender.h>
#include <2702math.h>
#include <cameraserver/CameraServer.h>
#include <frc/filter/MedianFilter.h>
#include <ObjectDetectionConfig.h>
#include <ObjectDetectionSettings.pb.h>
#include <AsyncVisionLog.h>
#include <ObjectFrameProcessorOpts.h>

/** @brief Contains the object detection image processing classes. Main is located in object.cpp
* Depends on:
* - wpilib ntcore
* - opencv4
*/

// Allows us to access M_PI and other constants from math.h
#define _USE_MATH_DEFINES

/** @brief UDP address to send object detection packets to. */
#define OBJECT_DETECTION_SERVER_IP "10.27.2.2"
/** @brief UDP port to send object detection packets to. */
#define OBJECT_DETECTION_SERVER_PORT 27021
// Structure classes where the serialization / deserialization functions are defined
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

/**
 * @class ObjectFrameProcessor
 * @brief Class for producing object detections from inputted frames
 * @details Runs a processing algorithm of filtering to extract only objects in a final frame and sending the positions and angles of those objects to the RoboRIO in order to be acted on
 * @tparam LatencyReporting Whether or not to print the latency to the filtered frame and the console
 * @tparam VideoDebugging Whether to add additional annotations to the frame for debugging
 * @tparam UseGui Whether to display the GUI replay elements like trackbars for certain config values and other elements
 * @tparam MultipleObjectDetection Whether to turn on or off the ability to detect multiple objects in a single frame. When off, the biggest element is selected to be the only potential object in thef rame. When on, all objects are treated as potential objects
 * @tparam SendPackets Whether to attempt to send packets to the roboRIO for it to use
 * @tparam UseOutputFiltering Whether to filter the outputted object distances and angles through @ref m_distanceFilter and @ref m_angleFilter before sendingt to the roboRIO
 */
template<ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui, ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering UseOutputFiltering>
class ObjectFrameProcessor : public FrmProcessor
{
	public:
	ObjectDetectionConfig<ObjectDetectionSettings> m_config;
	private:
    /** @brief The width of frames being fed into this object */
	int			m_frameWidth;
    /** @brief The height of frames being fed into this object */
	int			m_frameHeight;

	std::shared_ptr<nt::NetworkTable> m_nObjectDetectionNetTable = nt::NetworkTableInstance::GetDefault().GetTable("ObjectDetection");
	nt::DoublePublisher m_nLastCoralDistanceTopic = m_nObjectDetectionNetTable->GetDoubleTopic("CoralDistance").Publish();
	nt::DoublePublisher m_nLastCoralAngleTopic = m_nObjectDetectionNetTable->GetDoubleTopic("CoralAngle").Publish();

	nt::IntegerPublisher m_nLastLatencyTopic = m_nObjectDetectionNetTable->GetIntegerTopic("Latency").Publish();

	double		distance;

	/** @brief Camera calibration matrix. */
	cv::Mat mtx = (cv::Mat_<float>(3, 3) << 601.9259931202599, 0, 321.8862267762323,
		0, 595.9193354697203, 258.3875534160956,
		0, 0, 1);

	/** @brief Camera distortion coefficients. */
	cv::Mat dist = (cv::Mat_<float>(1, 5) << 0.1002088738641385, -0.1862351027260573, -0.006324181581118907, 0.006180225582097748, 0.1135952943832695);
	/** @brief Focal length of camera. */
	double		 m_focalLen;
	cv::Point2d CAMERA_CENTER;
    /** @brief Vector representing difference from the origin of the robot to the origin of the camera's view */
	Rebels::Point3d m_p3dCam2Robot;

    /** @brief The filter which distances are run through to help reduce extranneous values */
	frc::MedianFilter<double> m_distanceFilter;
    /** @brief The filter which distances are run through to help reduce extranneous values */
	frc::MedianFilter<double> m_angleFilter;

	/** @brief UDP packet sender. */
	UDPSender udpSender;

	nt::NetworkTableListener ntl = nt::NetworkTableListener::CreateTimeSyncListener(nt::NetworkTableInstance::GetDefault(), true, timesynccallback);

	/** @brief Thread lock for the camera server output. */
	std::mutex	m_mtxCS;
	/** @brief Bool to enable running setup of camera server only on first frame. */
	bool		m_bFirstVid = true;
	/** @brief Camera server output stream for the filtered frame. */
	cs::CvSource 	m_outputStream;
	/** @brief Camera server output stream for the raw frame. */
	cs::CvSource    m_outputStreamRaw;

	/** @brief Logger for raw frames inputted into the algorithm */
	std::unique_ptr<AsyncVisionLog> m_unfilteredLogger;

	static void onTrackbarSetHueMin(int newPos, void* inst);

	static void onTrackbarSetHueMax(int newPos, void* inst);

	static void onTrackbarSetSatMin(int newPos, void* inst);

	static void onTrackbarSetSatMax(int newPos, void* inst);

	static void onTrackbarSetValMin(int newPos, void* inst);

	static void onTrackbarSetValMax(int newPos, void* inst);

public:
	static int64_t m_i64TimeOffset;

	ObjectFrameProcessor(int frameWidth, int frameHeight);

	/** @brief Start logging frames to logfile. */
	void vStartLogging();

	int trackbarHueMin;
	int trackbarHueMax;
	int trackbarSatMin;
	int trackbarSatMax;
	int trackbarValMin;
	int trackbarValMax;

	/*!
	* @brief Sets hue min object detection parameter.
	* @param  newValue - integer representing the new hue min value
	*/
	void setPVHueMin(const int32_t& newValue);
	/*!
	* @brief Sets hue max object detection parameter.
	* @param  newValue - integer representing the new hue max value
	*/
	void setPVHueMax(const int32_t& newValue);
	/*!
	* @brief Sets saturation min object detection parameter.
	* @param  newValue - integer representing the new saturation min value
	*/
	void setSaturationMin(const int32_t& newValue);
	/*!
	* @brief Sets saturation max object detection parameter.
	* @param  newValue - integer representing the new saturation max value
	*/
	void setSaturationMax(const int32_t& newValue);
	/*!
	* @brief Sets value min object detection parameter.
	* @param  newValue - integer representing the new value min value
	*/
	void setValueMin(const int32_t& newValue);
	/*!
	* @brief Sets value max object detection parameter.
	* @param  newValue - integer representing the new value max value
	*/
	void setValueMax(const int32_t& newValue);

	virtual ~ObjectFrameProcessor();
	/** @brief Handles a time sync event from networkTables.
	* @param e - The networktables event.
	* @returns Nothing.
	*/
	static void timesynccallback(nt::Event const& e);

	/** @brief Send an @ref ObjectDetectionPacket .
	* @param packet - Packet to send.
	*/
	void SendObjectDetectionPacket(const ObjectDetectionPacket& packet);

	/*!
	* @brief Filters the image into a mask where each pixel is either able to be part of a valid detection(white) or not(black).
	* To tune these values, change them in the processorSettings.txt file.
	* @param frame - The raw input frame from the camera.
	* @param pv_hue_min - If a pixel's hue is lower than this, it will be not valid.
	* @param pv_hue_max - If a pixel's hue is higher than this, it will not be valid.
	* @param saturation_min - If a pixel's saturation is lower than this, it will not be valid.
	* @param saturation_min - If a pixel's saturation is higher than this, it will not be valid.
	* @param value_min - If a pixel's brightness is lower than this, it will not be valid.
	* @param value_max - If a pixel's brightness is lower than this, it will not be valid.
	* @returns The filtered frame as a @ref cv::Mat.
	*/
	cv::Mat filter_hsv_inverted(const cv::Mat& frame, int pv_hue_min, int pv_hue_max, int saturation_min, int saturation_max, int value_min, int value_max);

	/*!
	* @brief Returns object distance on the floor to the center of the robot in meters and the top plane angle to the center of the coral from the center of the robot in radians.
	* @param contour - The cylinder fitted contour as provided by @ref fit_cylinder_contour.
	* @param focal_length - The focal length of the camera.
	* @param actual_diamater - The actual diamater of the game piece(in this case the coral).
	* @param camera_center - The center of the camera as a Point2d, a constant.
	*/
	Rebels::Point2d calculate_coral_vector(const std::vector<cv::Point>& contour, double focal_length, double actual_diameter, const cv::Point2d& camera_center);

	/*!
	* @brief Takes a C-style array of 4 points and orders them starting from top-left going clockwise.
	* @param points - C-style array of 4 points unsorted
	* @returns A new C-style array of 4 points with the top-left point first then going clockwise.
	*/
	static std::array<cv::Point2f, 4> OrderPointArray(cv::Point2f* points);

	/*!
	* @brief Fits the contour provided to a cylinder shape.
	* @param contour - The input contour, as provided by @ref cv::findContours.
	* @param mask - The binary image where white is a valid part of a detection and black where it is not.
	* @param filteredFrame - The image to print any debugging text to.
	* @returns Empty pair if invalid contour, or a pair of the contour hull, and another pair of the major and minor axis endpoints(in that order) and the center point of the ellipse.
	*/
	std::tuple<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>, cv::Point> fit_cylinder_contour(const std::vector<cv::Point>& contour, cv::Mat mask);

	/** @brief Processes each frame, logs it to the logfile if @ref m_bIsLogging is set to true, runs the object detection on it, pushes the value to nettables,
	* @brief and sends the packet with the data to the roboRIO if @ref SEND_PACKETS or @ref PRODUCTION is true(and there is a valid detection).
	* @param nCam - The number of the camera, unused for object detection since it is a single-camera system.
	* @param pcf - A pointer to the @ref CameraFrame to be processed.
	* @returns Nothing.
	*/
	virtual void ProcessFrame(int nCam, CameraFrame const* pcf);

	/** @brief Processes each frame, logs it to the logfile if @ref m_bIsLogging is set to true, runs the object detection on it, pushes the value to nettables,
	* @brief and sends the packet with the data to the roboRIO if @ref SEND_PACKETS or @ref PRODUCTION is true(and there is a valid detection).
	* @param nCam - The number of the camera, unused for object detection since it is a single-camera system.
	* @param pcf - A pointer to the @ref CameraFrame to be processed.
	* @returns The filtered frame.
	*/
	cv::Mat ProcessFrameWithRet(int nCam, CameraFrame const* pcf);
};