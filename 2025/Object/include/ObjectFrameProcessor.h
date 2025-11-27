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
#include <VisionLog.h>
#include <cameraserver/CameraServer.h>

/*! \brief Contains the object detection image processing classes. Main is located in object.cpp
* Depends on:
* - wpilib ntcore
* - opencv4
*/

// Allows us to access M_PI and other constants from math.h
#define _USE_MATH_DEFINES

#ifndef IMGPROC_GUI
#define IMGPROC_GUI	0
#endif
/*! \brief UDP address to send object detection packets to. */
#define OBJECT_DETECTION_SERVER_IP "10.27.2.2"
/*! \brief UDP port to send object detection packets to. */
#define OBJECT_DETECTION_SERVER_PORT "27021"
/*! \brief Prints the latency to console. */
#ifndef DEBUG
#define DEBUG 1
#endif
/*! \brief Performs all the actions of \ref DEBUG plus annotating the video stream and sending it to the camera server for viewing in the browser and writes distance and angle values to .txt files in the directory, does nothing if DEBUG is off*/
#ifndef DEBUG_VIDEO
#define DEBUG_VIDEO 1
#endif
/*! \brief Enables or disables the sending of UDP packets to the server. Should be disabled when the server is not active. */
#ifndef SEND_PACKETS
#define SEND_PACKETS 1
#endif
/*! \brief Disables all debug flags and enables all features. */
#ifndef PRODUCTION
#define PRODUCTION 0
#endif
/*! \brief Whether or not to use median filtering for the output distance and angle values. */
#define USE_OUTPUT_FILTERING 1
/*! \brief Whether or not the camera should detect multiple objects. */
#define NO_MULTIPLE_OBJECT_DETECTION 0
// Structure classes where the serialization / deserialization functions are defined
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

class ObjectFrameProcessor : public FrmProcessor
{
	/*! \brief Hue min object detection parameter. */
	int			m_nHLow = 0;
	/*! \brief Saturation min object detection parameter. */
	int			m_nSLow = 0;
	/*! \brief Value min object detection parameter. */
	int			m_nVLow = 215;
	/*! \brief Hue max object detection parameter. */
	int			m_nHHigh = 30;
	/*! \brief Saturation max object detection parameter. */
	int			m_nSHigh = 30;
	/*! \brief Value max object detection parameter. */
	int			m_nVHigh = 255;
	/*! \brief Minimum pixel width of an object required for detection. */
	int			m_minPixelWidth = 20;
	/*! \brief Minimum pixel height of an object required for detection. */
	int			m_minPixelHeight = 25;
	/*! \brief Maximum pixel width of an object for detection. */
	int			m_maxPixelWidth = 500;
	/*! \brief Maximum pixel height of an object for detection. */
	int			m_maxPixelHeight = 350;
	/*! \brief Minimum pixel area of an object for detection. */
	int			m_minPixelArea = 4500;
	/*! \brief Maximum pixel area of an object for detection. */
	int			m_maxPixelArea = 115000;
	/*! \brief Minimum percentage of pixels that have to be valid inside of a detection for it to be valid. */
	double		m_minPercentFill = 0.45;
	/*! \brief X adjustment in meters from center of camera to center of robot. */
	double		m_camOffsetX = 0.22225;
	/*! \brief Y adjustment in meters from center of camera to center of robot. */
	double		m_camOffsetY = 0.33655;
	/*! \brief Center of camera to center of robot height adjustment in meters. */
	double		m_camHeight = 0.5334;
	/*! \brief Angle of camera below the horizontal in radians. */
	double		m_camAngle = 26 * M_PI / 180;
	/*! \brief Number of packets over which the median is taken for the distance / angle measurements. */
	int			m_filterPacketWindow = 8;

	std::string m_logDir;

	int			m_frameWidth;
	int			m_frameHeight;

	std::shared_ptr<nt::NetworkTable> m_nObjectDetectionNetTable = nt::NetworkTableInstance::GetDefault().GetTable("ObjectDetection");
	nt::DoublePublisher m_nLastCoralDistanceTopic = m_nObjectDetectionNetTable->GetDoubleTopic("CoralDistance").Publish();
	nt::DoublePublisher m_nLastCoralAngleTopic = m_nObjectDetectionNetTable->GetDoubleTopic("CoralAngle").Publish();

#if DEBUG && !PRODUCTION
	nt::IntegerPublisher m_nLastLatencyTopic = m_nObjectDetectionNetTable->GetIntegerTopic("Latency").Publish();

#endif

	double		m_fCDiam = 0.1016;

	double		distance;

	/*! \brief Camera calibration matrix. */
	cv::Mat mtx = (cv::Mat_<float>(3, 3) << 601.9259931202599, 0, 321.8862267762323,
		0, 595.9193354697203, 258.3875534160956,
		0, 0, 1);

	/*! \brief Camera distortion coefficients. */
	cv::Mat dist = (cv::Mat_<float>(1, 5) << 0.1002088738641385, -0.1862351027260573, -0.006324181581118907, 0.006180225582097748, 0.1135952943832695);
	/*! \brief Focal length of camera. */
	double		 m_focalLen;
	cv::Point2d CAMERA_CENTER;
	Rebels::Point3d m_p3dCam2Robot;

	frc::MedianFilter<double> m_distanceFilter;
	frc::MedianFilter<double> m_angleFilter;

#if SEND_PACKETS || PRODUCTION
	/*! \brief UDP packet sender. */
	CUDPSender udpSender;

	nt::NetworkTableListener ntl = nt::NetworkTableListener::CreateTimeSyncListener(nt::NetworkTableInstance::GetDefault(), true, timesynccallback);
#endif

#if DEBUG_VIDEO && !PRODUCTION
	/*! \brief Thread lock for the camera server output. */
	std::mutex	m_mtxCS;
	/*! \brief Bool to enable running setup of camera server only on first frame. */
	bool		m_bFirstVid = true;
	/*! \brief Camera server output stream for the filtered frame. */
	cs::CvSource 	m_outputStream;
	/*! \brief Camera server output stream for the raw frame. */
	cs::CvSource    m_outputStreamRaw;
	/*! \brief File that will be written with the processed distances. */
	std::ofstream m_distFile;
	/*! \brief File that will be written with the processed angles. */
	std::ofstream m_angleFile;
	/*! \brief File that will be written with the latency of each frame. */
	std::ofstream m_latencyFile;
#endif

	/*! \brief Thread lock for logfile output. */
	std::mutex		m_mtxLog;
	/*! \brief Logger for frames. */
	VisionLog		m_vl;

	/*! \brief Whether or not the incoming frames are being logged to the .log file. */
	bool			m_bIsLogging = false;
	/*! \brief Is true when logging is complete, as to not write any more frames to the file. */
	bool			m_bLoggingDone = false;
	/*! \brief The time when the \ref m_bLoggingDone was set to true. */
	time_t			m_tmLogEnd;

public:
	static int64_t m_i64TimeOffset;

	ObjectFrameProcessor(std::string logDir, int frameWidth, int frameHeight);

	/*! \brief Start logging frames to logfile. */
	virtual void vStartLogging();

	/*!
	* \brief Sets hue min object detection parameter.
	* \param  newValue - integer representing the new hue min value
	*/
	virtual void setPVHueMin(int newValue);
	/*!
	* \brief Sets hue max object detection parameter.
	* \param  newValue - integer representing the new hue max value
	*/
	virtual void setPVHueMax(int newValue);
	/*!
	* \brief Sets saturation min object detection parameter.
	* \param  newValue - integer representing the new saturation min value
	*/
	virtual void setSaturationMin(int newValue);
	/*!
	* \brief Sets saturation max object detection parameter.
	* \param  newValue - integer representing the new saturation max value
	*/
	virtual void setSaturationMax(int newValue);
	/*!
	* \brief Sets value min object detection parameter.
	* \param  newValue - integer representing the new value min value
	*/
	virtual void setValueMin(int newValue);
	/*!
	* \brief Sets value max object detection parameter.
	* \param  newValue - integer representing the new value max value
	*/
	virtual void setValueMax(int newValue);
	/*!
	* \brief Sets min pixel width object detection parameter.
	* \param  newValue - integer representing the new min pixel width
	*/
	virtual void setMinPixelWidth(int newValue);
	/*!
	* \brief Sets min pixel height object detection parameter.
	* \param  newValue - integer representing the new min pixel height
	*/
	virtual void setMinPixelHeight(int newValue);
	/*!
	* \brief Sets min pixel area object detection parameter.
	* \param  newValue - integer representing the new min pixel area
	*/
	virtual void setMinPixelArea(int newValue);
	/*!
	* \brief Sets max pixel width object detection parameter.
	* \param  newValue - integer representing the new max pixel width
	*/
	virtual void setMaxPixelWidth(int newValue);
	/*!
	* \brief Sets max pixel height object detection parameter.
	* \param  newValue - integer representing the new max pixel height
	*/
	virtual void setMaxPixelHeight(int newValue);
	/*!
	* \brief Sets max pixel area object detection parameter.
	* \param  newValue - integer representing the new max pixel area
	*/
	virtual void setMaxPixelArea(int newValue);
	/*!
	* \brief Sets min percent fill object detection parameter.
	* \param  newValue - integer representing the new min percent fill
	*/
	virtual void setMinPercentFill(double newValue);
	/*!
	* \brief Sets the X distance between the center of the camera and the center of the robot frame.
	* \param newValue - new distance in meters
	*/
	virtual void setCamOffsetX(double newValue);
	/*!
	* \brief Sets the Y distance between the center of the camera and the center of the robot frame.
	* \param newValue - new distance in meters
	*/
	virtual void setCamOffsetY(double newValue);
	/*!
	* \brief Sets the vertical distance between the center of the camera and the center of the robot frame.
	* \param newValue - new distance in meters
	*/
	virtual void setCamHeight(double newValue);
	/*!
	* \brief Sets the angle below the horizontal of the camera
	* \param newValue - new angle value in radians
	*/
	virtual void setCamAngle(double newValue);
	/*!
	* \brief Sets the number of packets over which the median is taken for distance / angle measurements.
	* Calling this function resets the median.
	* \param newValue - the new number of packets
	*/
	virtual void setFilterWindow(int newValue);
	/*!
	* \brief Returns the hue min object detection parameter.
	*/
	virtual int getPVHueMin() const;
	/*!
	* \brief Returns the hue max object detection parameter.
	*/
	virtual int getPVHueMax() const;
	/*!
	* \brief Returns the saturation min object detection parameter.
	*/
	virtual int getSaturationMin() const;
	/*!
	* \brief Returns the saturation max object detection parameter.
	*/
	virtual int getSaturationMax() const;
	/*!
	* \brief Returns the value min object detection parameter.
	*/
	virtual int getValueMin() const;
	/*!
	* \brief Returns the value max object detection parameter.
	*/
	virtual int getValueMax() const;
	/*!
	* \brief Returns the min pixel width object detection parameter.
	*/
	virtual int getMinPixelWidth() const;
	/*!
	* \brief Returns the min pixel height object detection parameter.
	*/
	virtual int getMinPixelHeight() const;
	/*!
	* \brief Returns the max pixel width object detection parameter.
	*/
	virtual int getMaxPixelWidth() const;
	/*!
	* \brief Returns the max pixel height object detection parameter.
	*/
	virtual int getMaxPixelHeight() const;
	/*!
	* \brief Returns the min pixel area object detection parameter.
	*/
	virtual int getMinPixelArea() const;
	/*!
	* \brief Returns the max pixel area object detection parameter.
	*/
	virtual int getMaxPixelArea() const;
	/*!
	* \brief Returns the min percent fill object detection parameter.
	*/
	virtual double getMinPercentFill() const;
	/*!
	* \brief Returns the x offset of the camera center relative to the center of the robot in meters.
	*/
	virtual double getCamOffsetX() const;
	/*!
	* \brief Returns the y offset of the camera center relative to the center of the robot in meters.
	*/
	virtual double getCamOffsetY() const;
	/*!
	* \brief Returns the height offset of the camera center relative to the center of the robot in meters.
	*/
	virtual double getCamHeight() const;
	/*!
	* \brief Returns the angle below the horizontal of the camera in radians.
	*/
	virtual double getCamAngle() const;
	/*!
	* \brief Returns the number of packets over which the median is taken for distance / angle measurements.
	*/
	virtual int getFilterWindow() const;
	virtual ~ObjectFrameProcessor();
	/*! \brief Handles a time sync event from networkTables.
	* \param e - The networktables event.
	* \returns Nothing.
	*/
	static void timesynccallback(nt::Event const& e);

	/*! \brief Send an \ref ObjectDetectionPacket .
	* \param packet - Packet to send.
	*/
	void SendObjectDetectionPacket(ObjectDetectionPacket packet);

	/*!
	* \brief Filters the image into a mask where each pixel is either able to be part of a valid detection(white) or not(black).
	* To tune these values, change them in the processorSettings.txt file.
	* \param frame - The raw input frame from the camera.
	* \param pv_hue_min - If a pixel's hue is lower than this, it will be not valid.
	* \param pv_hue_max - If a pixel's hue is higher than this, it will not be valid.
	* \param saturation_min - If a pixel's saturation is lower than this, it will not be valid.
	* \param saturation_min - If a pixel's saturation is higher than this, it will not be valid.
	* \param value_min - If a pixel's brightness is lower than this, it will not be valid.
	* \param value_max - If a pixel's brightness is lower than this, it will not be valid.
	* \returns The filtered frame as a \ref cv::Mat.
	*/
	cv::Mat filter_hsv_inverted(const cv::Mat& frame, int pv_hue_min, int pv_hue_max, int saturation_min, int saturation_max, int value_min, int value_max);

	/*!
	* \brief Returns object distance on the floor to the center of the robot in meters and the top plane angle to the center of the coral from the center of the robot in radians.
	* \param contour - The cylinder fitted contour as provided by \ref fit_cylinder_contour.
	* \param focal_length - The focal length of the camera.
	* \param actual_diamater - The actual diamater of the game piece(in this case the coral).
	* \param camera_center - The center of the camera as a Point2d, a constant.
	*/
	Rebels::Point2d calculate_coral_vector(const std::vector<cv::Point>& contour, double focal_length, double actual_diameter, const cv::Point2d& camera_center);

	/*!
	* \brief Takes a C-style array of 4 points and orders them starting from top-left going clockwise.
	* \param points - C-style array of 4 points unsorted
	* \returns A new C-style array of 4 points with the top-left point first then going clockwise.
	*/
	static std::array<cv::Point2f, 4> OrderPointArray(cv::Point2f* points);

	/*!
	* \brief Fits the contour provided to a cylinder shape.
	* \param contour - The input contour, as provided by \ref cv::findContours.
	* \param mask - The binary image where white is a valid part of a detection and black where it is not.
	* \param filteredFrame - The image to print any debugging text to.
	* \returns Empty pair if invalid contour, or a pair of the contour hull, and another pair of the major and minor axis endpoints(in that order).
	*/
	std::pair<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>> fit_cylinder_contour(const std::vector<cv::Point>& contour, cv::Mat mask, cv::Mat filteredFrame, int* heightPaintAt);

	/*! \brief Processes each frame, logs it to the logfile if \ref m_bIsLogging is set to true, runs the object detection on it, pushes the value to nettables,
	* \brief and sends the packet with the data to the roboRIO if \ref SEND_PACKETS or \ref PRODUCTION is true(and there is a valid detection).
	* \param nCam - The number of the camera, unused for object detection since it is a single-camera system.
	* \param pcf - A pointer to the \ref CameraFrame to be processed.
	* \returns Nothing.
	*/
	virtual void ProcessFrame(int nCam, CameraFrame const* pcf);

	/*! \brief Processes each frame, logs it to the logfile if \ref m_bIsLogging is set to true, runs the object detection on it, pushes the value to nettables,
	* \brief and sends the packet with the data to the roboRIO if \ref SEND_PACKETS or \ref PRODUCTION is true(and there is a valid detection).
	* \param nCam - The number of the camera, unused for object detection since it is a single-camera system.
	* \param pcf - A pointer to the \ref CameraFrame to be processed.
	* \returns The filtered frame.
	*/
	cv::Mat ProcessFrameWithRet(int nCam, CameraFrame const* pcf);
};