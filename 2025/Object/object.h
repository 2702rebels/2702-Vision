/*! \brief Contains the object detection image processing classes. Main is located in object.cpp
* Depends on:
* - wpilib ntcore
* - ObjectDetectionStruct.h and ObjectDetectionStruct.cpp
*/

#include <networktables/NetworkTableInstance.h>
#include <networktables/NetworkTable.h>
#include <networktables/DoubleTopic.h>
#include <networktables/IntegerTopic.h>
#include <networktables/BooleanTopic.h>
#include <sys/time.h>
#include "ObjectDetectionStruct.h"
#include "ObjectDetectionStruct.cpp"
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <span>

//#include <X11/Xlib.h>
#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <array>

#include <string>
#include <stdexcept>

#include <cameraserver/CameraServer.h>
#include <networktables/NetworkTableInstance.h>
#include <networktables/NetworkTableListener.h>
#include <drm/drm_fourcc.h>
#include <frc/filter/MedianFilter.h>
#include <units/time.h>
#include <wpimath/MathShared.h>
#include "rpicam.h"
#include <future>
#include <thread>
#include "VisLog.h"
#include <math.h>
#include "2702math.h"
#include "udpsender.h"

#pragma once
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
/*! \brief Directory where to log files to. */
#define LOG_DIR "/data/logs/"

#ifndef RESOLUTION_WIDTH
/*! \brief Width of the image taken. */
#define RESOLUTION_WIDTH 640
#endif

#ifndef RESOLUTION_HEIGHT
/*! \brief Width of the image taken. */
#define RESOLUTION_HEIGHT 480
#endif
// Structure classes where the serialization / deserialization functions are defined
using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

class imgProc : public FrmProcessor
{
	/*! \brief Hue min object detection parameter. */
	int			m_nHLow = 0;
	/*! \brief Saturation min object detection parameter. */
	int			m_nSLow = 0;
	/*! \brief Value min object detection parameter. */
	int			m_nVLow = 200;
	/*! \brief Hue max object detection parameter. */
	int			m_nHHigh = 20;
	/*! \brief Saturation max object detection parameter. */
	int			m_nSHigh = 20;
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
	double		m_minPercentFill = 0.2;
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

	shared_ptr<nt::NetworkTable>		m_nObjectDetectionNetTable = nt::NetworkTableInstance::GetDefault().GetTable("ObjectDetection");
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
	
	imgProc() :
		FrmProcessor(),
   		CAMERA_CENTER(mtx.at<float>(0, 2), mtx.at<float>(1, 2)),
		m_p3dCam2Robot(m_camOffsetX, m_camOffsetY, m_camHeight),
		m_distanceFilter(m_filterPacketWindow),
		m_angleFilter(m_filterPacketWindow)
	{
		m_focalLen = (mtx.at<float>(0, 0) + mtx.at<float>(1, 1)) / 2;

    std::cout << "Camera Matrix: " << mtx << endl;
    std::cout << "Focal Length Coefficients: " << mtx.at<float>(0,0) << "    " << mtx.at<float>(1,2) << endl;
    std::cout << "Focal Length: " << m_focalLen << endl;
	#if (DEBUG_VIDEO || IMGPROC_GUI) && !PRODUCTION
	m_angleFile.open("./angle.txt");
	m_distFile.open("./dist.txt");
	m_latencyFile.open("./latency.txt");
#endif
#if (IMGPROC_GUI && !PRODUCTION)
		cv::namedWindow("Filtered Video");

		cv::createTrackbar("PV Hue Min", "Filtered Video", &m_nHLow, 360);
		cv::createTrackbar("PV Hue Max", "Filtered Video", &m_nHHigh, 360);
		cv::createTrackbar("Sat Min", "Filtered Video", &m_nSLow, 255);
		cv::createTrackbar("Sat Max", "Filtered Video", &m_nSHigh, 255);
		cv::createTrackbar("Val Min", "Filtered Video", &m_nVLow, 255);
		cv::createTrackbar("Val Max", "Filtered Video", &m_nVHigh, 255);
#endif
	std::cout << "Reached end of constructor\n";
	}

	/*! \brief Start logging frames to logfile. */
	virtual void vStartLogging(){
		m_bIsLogging = true;
	}

	/*! 
	* \brief Sets hue min object detection parameter. 
	* \param  newValue - integer representing the new hue min value
	*/
	virtual void setPVHueMin(int newValue)
	{
		m_nHLow = newValue;
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::setTrackbarPos("PV Hue Min", "Filtered Video", newValue);
		#endif
	}
	/*! 
	* \brief Sets hue max object detection parameter.
	* \param  newValue - integer representing the new hue max value
	*/
	virtual void setPVHueMax(int newValue)
	{
		m_nHHigh = newValue;
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::setTrackbarPos("PV Hue Max", "Filtered Video", newValue);
		#endif
	}
	/*! 
	* \brief Sets saturation min object detection parameter.
	* \param  newValue - integer representing the new saturation min value
	*/
	virtual void setSaturationMin(int newValue)
	{
		m_nSLow = newValue;
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::setTrackbarPos("Sat Min", "Filtered Video", newValue);
		#endif
	}
	/*! 
	* \brief Sets saturation max object detection parameter.
	* \param  newValue - integer representing the new saturation max value
	*/
	virtual void setSaturationMax(int newValue)
	{
		m_nSHigh = newValue;
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::setTrackbarPos("Sat Max", "Filtered Video", newValue);
		#endif
	}
	/*! 
	* \brief Sets value min object detection parameter.
	* \param  newValue - integer representing the new value min value
	*/
	virtual void setValueMin(int newValue)
	{
		m_nVLow = newValue;
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::setTrackbarPos("Val Min", "Filtered Video", newValue);
		#endif
	}
	/*! 
	* \brief Sets value max object detection parameter.
	* \param  newValue - integer representing the new value max value
	*/
	virtual void setValueMax(int newValue)
	{
		m_nVHigh = newValue;
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::setTrackbarPos("Val Max", "Filtered Video", newValue);
		#endif
	}
	/*! 
	* \brief Sets min pixel width object detection parameter.
	* \param  newValue - integer representing the new min pixel width
	*/
	virtual void setMinPixelWidth(int newValue)
	{
		m_minPixelWidth = newValue;
	}
	/*! 
	* \brief Sets min pixel height object detection parameter.
	* \param  newValue - integer representing the new min pixel height
	*/
	virtual void setMinPixelHeight(int newValue)
	{
		m_minPixelHeight = newValue;
	}
	/*! 
	* \brief Sets min pixel area object detection parameter.
	* \param  newValue - integer representing the new min pixel area
	*/
	virtual void setMinPixelArea(int newValue)
	{
		m_minPixelArea = newValue;
	}
	/*! 
	* \brief Sets max pixel width object detection parameter.
	* \param  newValue - integer representing the new max pixel width
	*/
	virtual void setMaxPixelWidth(int newValue)
	{
		m_maxPixelWidth = newValue;
	}
	/*! 
	* \brief Sets max pixel height object detection parameter.
	* \param  newValue - integer representing the new max pixel height
	*/
	virtual void setMaxPixelHeight(int newValue)
	{
		m_maxPixelHeight = newValue;
	}
	/*! 
	* \brief Sets max pixel area object detection parameter.
	* \param  newValue - integer representing the new max pixel area
	*/
	virtual void setMaxPixelArea(int newValue)
	{
		m_maxPixelArea = newValue;
	}
	/*! 
	* \brief Sets min percent fill object detection parameter.
	* \param  newValue - integer representing the new min percent fill
	*/
	virtual void setMinPercentFill(double newValue)
	{
		m_minPercentFill = newValue;
	}
	/*!
	* \brief Sets the X distance between the center of the camera and the center of the robot frame. 
	* \param newValue - new distance in meters
	*/
	virtual void setCamOffsetX(double newValue)
	{
		m_camOffsetX = newValue;
		m_p3dCam2Robot = Rebels::Point3d(m_camOffsetX, m_camOffsetY, m_camHeight);
	}
	/*!
	* \brief Sets the Y distance between the center of the camera and the center of the robot frame.
	* \param newValue - new distance in meters
	*/
	virtual void setCamOffsetY(double newValue)
	{
		m_camOffsetY = newValue;
		m_p3dCam2Robot = Rebels::Point3d(m_camOffsetX, m_camOffsetY, m_camHeight);
	}
	/*!
	* \brief Sets the vertical distance between the center of the camera and the center of the robot frame.
	* \param newValue - new distance in meters
	*/
	virtual void setCamHeight(double newValue)
	{
		m_camHeight = newValue;
		m_p3dCam2Robot = Rebels::Point3d(m_camOffsetX, m_camOffsetY, m_camHeight);
	}
	/*!
	* \brief Sets the angle below the horizontal of the camera
	* \param newValue - new angle value in radians
	*/
	virtual void setCamAngle(double newValue)
	{
		m_camAngle = newValue;
	}
	/*!
	* \brief Sets the number of packets over which the median is taken for distance / angle measurements.
	* Calling this function resets the median.
	* \param newValue - the new number of packets
	*/
	virtual void setFilterWindow(int newValue)
	{
		if(newValue == m_filterPacketWindow){
			// Avoid unnecessary filtering resets.
			return;
		}
		m_filterPacketWindow = newValue;
		frc::MedianFilter<double> m_distanceFilter = frc::MedianFilter<double>(m_filterPacketWindow);
		frc::MedianFilter<double> m_angleFilter = frc::MedianFilter<double>(m_filterPacketWindow);
	}
	/*! 
	* \brief Returns the hue min object detection parameter. 
	*/
	virtual int getPVHueMin() const
	{
		return m_nHLow;
	}
	/*! 
	* \brief Returns the hue max object detection parameter. 
	*/
	virtual int getPVHueMax() const
	{
		return m_nHHigh;
	}
	/*! 
	* \brief Returns the saturation min object detection parameter. 
	*/
	virtual int getSaturationMin() const
	{
		return m_nSLow;
	}
	/*! 
	* \brief Returns the saturation max object detection parameter. 
	*/
	virtual int getSaturationMax() const
	{
		return m_nSHigh;
	}
	/*! 
	* \brief Returns the value min object detection parameter. 
	*/
	virtual int getValueMin() const
	{
		return m_nVLow;
	}
	/*! 
	* \brief Returns the value max object detection parameter. 
	*/
	virtual int getValueMax() const
	{
		return m_nVHigh;
	}
	/*! 
	* \brief Returns the min pixel width object detection parameter. 
	*/
	virtual int getMinPixelWidth() const
	{
		return m_minPixelWidth;
	}
	/*! 
	* \brief Returns the min pixel height object detection parameter. 
	*/
	virtual int getMinPixelHeight() const
	{
		return m_minPixelHeight;
	}
	/*! 
	* \brief Returns the max pixel width object detection parameter. 
	*/
	virtual int getMaxPixelWidth() const
	{
		return m_maxPixelWidth;
	}
	/*! 
	* \brief Returns the max pixel height object detection parameter. 
	*/
	virtual int getMaxPixelHeight() const
	{
		return m_maxPixelHeight;
	}
	/*! 
	* \brief Returns the min pixel area object detection parameter. 
	*/
	virtual int getMinPixelArea() const
	{
		return m_minPixelArea;
	}
	/*! 
	* \brief Returns the max pixel area object detection parameter. 
	*/
	virtual int getMaxPixelArea() const
	{
		return m_maxPixelArea;
	}
	/*! 
	* \brief Returns the min percent fill object detection parameter. 
	*/
	virtual double getMinPercentFill() const
	{
		return m_minPercentFill;
	}
	/*!
	* \brief Returns the x offset of the camera center relative to the center of the robot in meters.
	*/
	virtual double getCamOffsetX() const
	{
		return m_camOffsetX;
	}
	/*!
	* \brief Returns the y offset of the camera center relative to the center of the robot in meters.
	*/
	virtual double getCamOffsetY() const
	{
		return m_camOffsetY;
	}
	/*!
	* \brief Returns the height offset of the camera center relative to the center of the robot in meters.
	*/
	virtual double getCamHeight() const
	{
		return m_camHeight;
	}
	/*!
	* \brief Returns the angle below the horizontal of the camera in radians.
	*/
	virtual double getCamAngle() const
	{
		return m_camAngle;
	}
	/*!
	* \brief Returns the number of packets over which the median is taken for distance / angle measurements.
	*/
	virtual int getFilterWindow() const
	{
		return m_filterPacketWindow;
	}
	virtual ~imgProc()
	{

	}
	/*! \brief Handles a time sync event from networkTables. 
	* \param e - The networktables event.
	* \returns Nothing.
	*/
	static void timesynccallback( nt::Event const &e )
	{
		if (e.flags & NT_EVENT_TIMESYNC) {
			printf( "tscb::: Have TIMESYNC\n" );
			nt::TimeSyncEventData const *ptsed = e.GetTimeSyncEventData();
			imgProc::m_i64TimeOffset = ptsed->serverTimeOffset;
			printf( "Got Offset -> %ld\n", ptsed->serverTimeOffset );
		}
		else {
			printf( "tscb::: -> %x\n", e.flags );
		}
	}

	/*! \brief Send an \ref ObjectDetectionPacket .
	* \param packet - Packet to send.
	*/
	void SendObjectDetectionPacket(ObjectDetectionPacket packet){
		#if SEND_PACKETS || PRODUCTION
		try{
		std::vector<uint8_t> bytesVec(ObjectDetectionPacketStructType::GetSize());
		std::span<uint8_t> bytes(bytesVec.data(), bytesVec.size());
		ObjectDetectionPacketStructType::Pack(bytes, packet);
		udpSender.bSend(bytes);
		}
		catch(std::exception &e){
			std::cerr << "Failed to send packet with an exception: " << e.what() << "\n";
		}
		#endif
	}

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
	cv::Mat filter_hsv_inverted(const cv::Mat& frame, int pv_hue_min, int pv_hue_max, int saturation_min, int saturation_max, int value_min, int value_max)
	{
		cv::Mat hsv, mask;
		cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

		auto invert_and_scale_hue = [](int pv_hue) {
			int inverted_hue = (pv_hue != 0) ? 360 - pv_hue : 0;
			return inverted_hue / 2;
		};

		int cv_hue_min = invert_and_scale_hue(pv_hue_min);
		int cv_hue_max = invert_and_scale_hue(pv_hue_max);

		if (cv_hue_min > cv_hue_max) {
			std::swap(cv_hue_min, cv_hue_max);
		}

		cv::Scalar lower_bound(cv_hue_min, saturation_min, value_min);
		cv::Scalar upper_bound(cv_hue_max, saturation_max, value_max);
		cv::inRange(hsv, lower_bound, upper_bound, mask);
		return mask;
	}
	
	/*! 
	* \brief Returns object distance on the floor to the center of the robot in meters and the top plane angle to the center of the coral from the center of the robot in radians.
	* \param contour - The cylinder fitted contour as provided by \ref fit_cylinder_contour.
	* \param focal_length - The focal length of the camera.
	* \param actual_diamater - The actual diamater of the game piece(in this case the coral).
	* \param camera_center - The center of the camera as a Point2d, a constant.
	*/
	Rebels::Point2d calculate_coral_vector(const std::vector<cv::Point>& contour, double focal_length, double actual_diameter, const cv::Point2d& camera_center)
	{
		cv::RotatedRect rect = cv::fitEllipse(contour);
        // Calculate the vector from the sensor to the object
        Rebels::Point3d v( 1, -(rect.center.x - mtx.at<float>(0, 2)) / mtx.at<float>(0, 0), -(rect.center.y - mtx.at<float>(1,2)) / mtx.at<float>(1, 1) );

        // The Camera is tiled down, so rotate the vector
        // to "Robot Space"
		Rebels::Quaternion qCamTilt(m_camAngle, Rebels::Point3d(0., 1., 0.));
        Rebels::Point3d vRot = qCamTilt * v;

        // Find the Z intercept -- the point where the vector touches the floor
        double fT = -m_p3dCam2Robot.z() / vRot.z();

        // Now compute the location of the point
        Rebels::Point3d pos = vRot * fT + m_p3dCam2Robot;
		return Rebels::Point2d(pos.x(), pos.y());
	}

	/*!
	* \brief Takes a C-style array of 4 points and orders them starting from top-left going clockwise. 
	* \param points - C-style array of 4 points unsorted
	* \returns A new C-style array of 4 points with the top-left point first then going clockwise. 
	*/
	static std::array<cv::Point2f, 4> OrderPointArray(cv::Point2f *points){
		int highestIndex = 0;
		int max = 0;
		int lowestIndex = 0;
		int min = INFINITY;
		// Find top-right point
		for(int i = 0; i < 4; i++){
			if(points[i].x + points[i].y > max){
				highestIndex = i;
				max = points[i].x + points[i].y;
			}
			if(points[i].x + points[i].y < min){
				lowestIndex = i;
				min = points[i].x + points[i].y;
			}
		}
		std::array<cv::Point2f, 4> pointsSorted;
		pointsSorted[1] = points[highestIndex];
		pointsSorted[3] = points[lowestIndex];
		std::vector<int> indexes = {0,1,2,3};
		indexes.erase(std::remove(indexes.begin(), indexes.end(), highestIndex), indexes.end());
		indexes.erase(std::remove(indexes.begin(), indexes.end(), lowestIndex), indexes.end());
		if(points[indexes[0]].x > points[indexes[1]].x){
			pointsSorted[2] = points[indexes[0]];
			pointsSorted[0] = points[indexes[1]];
		}
		else{
			pointsSorted[2] = points[indexes[1]];
			pointsSorted[0] = points[indexes[0]];
		}
		return pointsSorted;
	}

	/*!
	* \brief Fits the contour provided to a cylinder shape.
	* \param contour - The input contour, as provided by \ref cv::findContours.
	* \param mask - The binary image where white is a valid part of a detection and black where it is not.
	* \param filteredFrame - The image to print any debugging text to.
	* \returns Empty pair if invalid contour, or a pair of the contour hull, and another pair of the major and minor axis endpoints(in that order).
	*/
	std::pair<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>> fit_cylinder_contour(const std::vector<cv::Point>& contour, cv::Mat mask, cv::Mat filteredFrame, int *heightPaintAt)
	{
		if (contour.size() < 5) {
			return { {}, {} };
		}

		cv::RotatedRect ellipse = cv::fitEllipse(contour);
		cv::Rect rect = ellipse.boundingRect();
		cv::Point2f points[4];
		ellipse.points(points);
		std::array<cv::Point2f, 4> pointsSorted = OrderPointArray(points);
		// Sort points to get top-left in right spot
		cv::Point center = (rect.br() + rect.tl()) * 0.5;
		float majorAxisLength = std::max(rect.width, rect.height);
		if(majorAxisLength < m_minPixelWidth || majorAxisLength > m_maxPixelWidth)
		{
			return { {}, {} };
		}
		float minorAxisLength = std::min(rect.width, rect.height);
		if(minorAxisLength < m_minPixelHeight || minorAxisLength > m_maxPixelHeight){
			return { {}, {} };
		}
		double area = rect.width * rect.height;
		if(area < m_minPixelArea || area > m_maxPixelArea){
			return { {}, {} };
		}
		// suspicious... the lines are coming out wrong the points must be goofed
		Rebels::LinearSlope line1(pointsSorted[0].x, pointsSorted[0].y, pointsSorted[1].x, pointsSorted[1].y); // top
		Rebels::LinearSlope line2(pointsSorted[1].x, pointsSorted[1].y, pointsSorted[2].x, pointsSorted[2].y); // right
		Rebels::LinearSlope line3(pointsSorted[2].x, pointsSorted[2].y, pointsSorted[3].x, pointsSorted[3].y); // bot
		Rebels::LinearSlope line4(pointsSorted[3].x, pointsSorted[3].y, pointsSorted[0].x, pointsSorted[0].y); // left
		int pixels = 0;
		int validPixels = 0;
		int xMin = std::max(rect.x, 0);
		int yMin = std::max(rect.y, 0);
		int xMax = std::min(rect.width + rect.x, RESOLUTION_WIDTH - 1);
		int yMax = std::min(rect.height + rect.y, RESOLUTION_HEIGHT - 1);
		if(xMin > xMax){
			int buf = xMin;
			xMin = xMax;
			xMax = buf;
		}
		if(yMin > yMax){
			int buf = yMin;
			yMin = yMax;
			yMax = buf;
		}
		for(int x = xMin; x < xMax; x++){
			for(int y = yMin; y < yMax; y++){
				if(line1.IsPointBelowLine(x, y) && line2.IsPointBelowLine(x, y) && line3.IsPointAboveLine(x, y) && line4.IsPointBelowLine(x, y)){
					if(mask.at<uchar>(x, y) > 0){
						validPixels++;
					}
					pixels++;
				}
			}
		}
		double detectpercent = (double)validPixels / (double)pixels;
		printf("detectpercent: %f\n", detectpercent);
		printf("npixels: %d\n", pixels);
		printf("validpixels: %d\n", validPixels);
		if(detectpercent < m_minPercentFill){
			return { {}, {} };
		}
		std::vector<cv::Point> hull;
		cv::convexHull(contour, hull);
		float angle = ellipse.angle;

		// Convert angle to radians
		float angleRad = angle * CV_PI / 180.0;

		// Calculate the endpoints of the major axis
		cv::Point2f majorAxisEndPoint1(
			center.x + majorAxisLength / 2 * cos(angleRad),
			center.y + majorAxisLength / 2 * sin(angleRad)
		);

		cv::Point2f majorAxisEndPoint2(
			center.x - majorAxisLength / 2 * cos(angleRad),
			center.y - majorAxisLength / 2 * sin(angleRad)
		);

		#if (DEBUG_VIDEO || IMGPROC_GUI) && !PRODUCTION
		std::string uvtext = "(" + std::to_string(ellipse.center.x) + ", " + std::to_string(ellipse.center.y) + ")";
		cv::putText(filteredFrame, uvtext, cv::Point(10, *heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
		*heightPaintAt = *heightPaintAt + 30;
		#endif

		return { hull, { majorAxisEndPoint1, majorAxisEndPoint2 } };
	}

	/*! \brief Processes each frame, logs it to the logfile if \ref m_bIsLogging is set to true, runs the object detection on it, pushes the value to nettables, 
	* \brief and sends the packet with the data to the roboRIO if \ref SEND_PACKETS or \ref PRODUCTION is true(and there is a valid detection). 
	* \param nCam - The number of the camera, unused for object detection since it is a single-camera system.
	* \param pcf - A pointer to the \ref CameraFrame to be processed.
	* \returns Nothing.
	*/
	virtual void ProcessFrame(int nCam, CameraFrame const *pcf){
		ProcessFrameWithRet(nCam, pcf);
	}

	/*! \brief Processes each frame, logs it to the logfile if \ref m_bIsLogging is set to true, runs the object detection on it, pushes the value to nettables, 
	* \brief and sends the packet with the data to the roboRIO if \ref SEND_PACKETS or \ref PRODUCTION is true(and there is a valid detection). 
	* \param nCam - The number of the camera, unused for object detection since it is a single-camera system.
	* \param pcf - A pointer to the \ref CameraFrame to be processed.
	* \returns The filtered frame.
	*/
	cv::Mat ProcessFrameWithRet( int nCam, CameraFrame const *pcf )
	{
		printf( "Have Frame from %d:%d %dx%d\n", nCam, pcf->u32Sequence(), pcf->nRows(), pcf->nCols() );
		cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image() );
		int heightPaintAt = 60;
		if(m_mtxLog.try_lock()){
			if(m_bLoggingDone){
			}
			else if(m_vl.bIsOpen()){
				m_vl.bWriteRGB(frame, pcf->u32Sequence(), pcf->u32Exposure());
				if(time(0) > m_tmLogEnd){
					m_vl.vClose();
					m_bLoggingDone = true;
					printf("Finished logging\n");
				}
			}
			else{
				if(m_bIsLogging){
					try{
						char szTmp[128];
						for(int i = 0; ; i++){
							snprintf(szTmp, sizeof(szTmp), "%s/object-%04d.log", LOG_DIR, i);
							if(access(szTmp, F_OK) == -1){
								break;
							}
						}
						printf("Opening log file at %s\n", szTmp);
						m_vl.vOpen(szTmp, true);
						m_tmLogEnd = time(0) + 160;
					}
					catch(std::exception &e){
						m_bLoggingDone = true;
						printf("Unable to start logging: %s\n", e.what());
					}
				}
			}
			m_mtxLog.unlock();
		}
		#if (IMGPROC_GUI && !PRODUCTION)
		cv::imshow( "Raw", frame );
#endif
		// cv::Mat hsv( pcf->nRows(), pcf->nCols(), CV_8UC3);

		// cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

		// cv::Mat thresh;
		// cv::inRange(hsv, cv::Scalar( m_nHLow, m_nSLow, m_nVLow), cv::Scalar( m_nHHigh, m_nSHigh, m_nVHigh), thresh);

		//frame = cv::undistort_frame(frame, mtx, dist);
        cv::Mat mask = filter_hsv_inverted(frame, m_nHLow, m_nHHigh, m_nSLow, m_nSHigh, m_nVLow, m_nVHigh);
        cv::Mat filtered_frame;
        cv::bitwise_and(frame, frame, filtered_frame, mask);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	std::vector<ObjectDetectionInfo> corals;
        if (!contours.empty()) {
			for(int i = 0; i < contours.size(); i++){
            // Fit a contour and get endpoints
			#if NO_MULTIPLE_OBJECT_DETECTION
			auto largest_contour = std::max_element(contours.begin(), contours.end(),
			[](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
				return cv::contourArea(a) < cv::contourArea(b);
			});
			contours[i] = *largest_contour;
			#endif
            auto [fitted_contour, endpoints] = fit_cylinder_contour(contours[i], mask, filtered_frame, &heightPaintAt);
            if (!fitted_contour.empty()) {
                Rebels::Point2d coralVec = calculate_coral_vector(contours[i], m_focalLen, m_fCDiam, CAMERA_CENTER);
				double distance = coralVec.length();
				// u = x, v = y
				double angle = std::atan2(coralVec.v(), coralVec.u());
				printf( "%.3f   %.3f\n", distance, angle );
				// NaN compare operations are always false, so if either is NaN, don't use it.
				if(distance == distance && angle == angle){
				#if USE_OUTPUT_FILTERING
				distance = m_distanceFilter.Calculate(distance);
				angle = m_angleFilter.Calculate(angle);
				#endif
				m_nLastCoralDistanceTopic.SetDefault(distance);
				m_nLastCoralAngleTopic.SetDefault(angle);
				corals.push_back(ObjectDetectionInfo(distance, angle, (uint8_t)20, ObjectDetectionTypes::CORAL));
                // Draw the fitted contour
				#if (DEBUG_VIDEO || IMGPROC_GUI) && !PRODUCTION
				std::string coral_vec_text = "(" + std::to_string(coralVec.u()) + ", " + std::to_string(coralVec.v()) + ")";
				std::string distance_text = "Distance: " + std::to_string(distance / 0.0254) + "in";
				std::string angle_text = "Angle: " + std::to_string(angle * (180/M_PI)) + "deg";
				m_distFile << distance << "\n";
				m_angleFile << angle << "\n";
				cv::putText(filtered_frame, coral_vec_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
				heightPaintAt += 30;
				cv::putText(filtered_frame, distance_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
				heightPaintAt += 30;
				cv::putText(filtered_frame, angle_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
				heightPaintAt += 30;
                cv::drawContours(filtered_frame, std::vector<std::vector<cv::Point>>{fitted_contour}, -1, cv::Scalar(0, 255, 0), 2);
                // Draw the major axis line if endpoints are valid
                if (endpoints.first.x != 0 || endpoints.first.y != 0 || endpoints.second.x != 0 || endpoints.second.y != 0) {
                    cv::line(filtered_frame, endpoints.first, endpoints.second, cv::Scalar(255, 0, 0), 2);
                }
			#endif
				}
			}
			#if NO_MULTIPLE_OBJECT_DETECTION
			break;
			#endif
		}
				#if SEND_PACKETS || PRODUCTION
				struct timespec tsCur;
				struct timeval tvCur;
				struct timeval tvDelta;
			
				// Get current time since power on
				clock_gettime(CLOCK_MONOTONIC_RAW, &tsCur );
				TIMESPEC_TO_TIMEVAL( &tvCur, &tsCur );
				// Calculate delta between when frame was catured and now.
				timersub( &tvCur, pcf->ts(), &tvDelta );
				std::sort(corals.begin(), corals.end());
				int count = corals.size();
				while(corals.size() < 4){
					corals.push_back(ObjectDetectionInfo());
				}
				long time = nt::Now() + m_i64TimeOffset - tvDelta.tv_usec; // timestamp is at frame capture time in microseconds with NetworkTables server-adjusted time
				ObjectDetectionPacket packet(PACKET_KEY, PACKET_VERSION, time, count, corals.at(0), corals.at(1), corals.at(2), corals.at(3));
				SendObjectDetectionPacket(packet);
				#endif

	}
		
		#if DEBUG && !PRODUCTION
			struct timespec tsCur;
			struct timeval tvCur;
			struct timeval tvDelta;

			clock_gettime(CLOCK_MONOTONIC_RAW, &tsCur);
			TIMESPEC_TO_TIMEVAL(&tvCur, &tsCur);
			timersub(&tvCur, pcf->ts(), &tvDelta);
			m_latencyFile << tvDelta.tv_usec << "\n";
			m_nLastLatencyTopic.SetDefault(tvDelta.tv_usec);
		#if (DEBUG_VIDEO || IMGPROC_GUI) && !PRODUCTION
		// Display distance and angle (only if valid values were calculated)
			std::string latency_text = "Latency: " + std::to_string(tvDelta.tv_usec) + "us";
			std::string exposure_text = "Exposure: " + std::to_string(pcf->u32Exposure());
			cv::putText(filtered_frame, exposure_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);	
			heightPaintAt += 30;	
			cv::putText(filtered_frame, latency_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
			heightPaintAt += 30;
		if (m_mtxCS.try_lock()) { 
			if (m_bFirstVid) {
				m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows() );
				m_outputStreamRaw = frc::CameraServer::PutVideo("VisionRaw", pcf->nCols(), pcf->nRows());
				m_bFirstVid = false;
			}
			m_outputStream.PutFrame(filtered_frame);
			m_outputStreamRaw.PutFrame(frame);
			m_mtxCS.unlock();
		}
		#endif
		#endif


#if (IMGPROC_GUI && !PRODUCTION)
		// Display the results
   		cv::imshow("Filtered Video", filtered_frame);
        cv::imshow("Mask", mask);

		if (cv::waitKey(1) == 'q') {
			cv::destroyAllWindows();
			exit(0);
		}
#endif
		return filtered_frame;
	}
};

// Initialize static vars
int64_t imgProc::m_i64TimeOffset;