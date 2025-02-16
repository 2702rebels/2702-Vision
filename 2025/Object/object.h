#include <networktables/NetworkTableInstance.h>
#include <networktables/NetworkTable.h>
#include <networktables/DoubleTopic.h>
#include <asio.hpp>
using asio::ip::udp;
#include <sys/time.h>
#include "ObjectDetectionStruct.h"
#include "ObjectDetectionStruct.cpp"
#pragma once

#ifndef IMGPROC_GUI
#define IMGPROC_GUI	0
#endif
#define OBJECT_DETECTION_SERVER_IP "127.0.0.1"
#define OBJECT_DETECTION_SERVER_PORT "6666"

using StructTypeObjectDetectionInfo = wpi::Struct<ObjectDetectionInfo>;
using StructTypeObjectDetectionPacket = wpi::Struct<ObjectDetectionPacket>;

class imgProc : public FrmProcessor
{
	int			m_nHLow = 179;
	int			m_nSLow = 0;
	int			m_nVLow = 147;
	int			m_nHHigh = 180;
	int			m_nSHigh = 255;
	int			m_nVHigh = 255;

	shared_ptr<nt::NetworkTable>		m_nObjectDetectionNetTable = nt::NetworkTableInstance::GetDefault().GetTable("ObjectDetection");
	nt::DoublePublisher m_nLastCoralDistanceTopic = m_nObjectDetectionNetTable->GetDoubleTopic("ObjectDetection/CoralDistance").Publish();
	nt::DoublePublisher m_nLastCoralAngleTopic = m_nObjectDetectionNetTable->GetDoubleTopic("ObjectDetection/CoralAngle").Publish();

	double		m_fCDiam = 0.1016;

	double		distance;
	
	cv::Mat mtx = (cv::Mat_<float>(3, 3) << 888.7765991141953, 0, 304.4048631642603,
0, 892.4756367000551, 370.9657053548069,
0, 0, 1);

	cv::Mat dist = (cv::Mat_<float>(1, 5) << 0.1164723784381825, 0.5948511019477938, 0.0178130457349927, -0.03413649680380905, -1.650075067124663);
	double		 m_focalLen;
   	cv::Point2d CAMERA_CENTER;

	udp::socket m_objectDetectionDataSocket;
	udp::endpoint m_objectDetectionEndpoint;
	
public:
	imgProc() :
		FrmProcessor(),
   		CAMERA_CENTER(mtx.at<float>(0, 2), mtx.at<float>(1, 2))
	{
		m_focalLen = (mtx.at<float>(0, 0) + mtx.at<float>(1, 1)) / 2;

    std::cout << "Camera Matrix: " << mtx << endl;
    std::cout << "Focal Length Coefficients: " << mtx.at<float>(0,0) << "    " << mtx.at<float>(1,2) << endl;
    std::cout << "Focal Length: " << m_focalLen << endl;
	asio::io_context io_context;
	udp::resolver resolver(io_context);
	m_objectDetectionEndpoint = *resolver.resolve(udp::v4(), OBJECT_DETECTION_SERVER_IP, OBJECT_DETECTION_SERVER_PORT).begin();
	m_objectDetectionDataSocket = udp::socket(io_context);
	m_objectDetectionDataSocket.open(udp::v4());
#if (IMGPROC_GUI)
		cv::namedWindow("Filtered Video");

		cv::createTrackbar("PV Hue Min", "Filtered Video", &m_nHLow, 360);
		cv::createTrackbar("PV Hue Max", "Filtered Video", &m_nHHigh, 360);
		cv::createTrackbar("Sat Min", "Filtered Video", &m_nSLow, 255);
		cv::createTrackbar("Sat Max", "Filtered Video", &m_nSHigh, 255);
		cv::createTrackbar("Val Min", "Filtered Video", &m_nVLow, 255);
		cv::createTrackbar("Val Max", "Filtered Video", &m_nVHigh, 255);
#endif
	}
	virtual void setPVHueMin(int newValue)
	{
		m_nHLow = newValue;
		cv::setTrackbarPos("PV Hue Min", "Filtered Video", newValue);
	}
	virtual void setPVHueMax(int newValue)
	{
		m_nHHigh = newValue;
		cv::setTrackbarPos("PV Hue Max", "Filtered Video", newValue);
	}
	virtual void setSaturationMin(int newValue)
	{
		m_nSLow = newValue;
		cv::setTrackbarPos("Sat Min", "Filtered Video", newValue);
	}
	virtual void setSaturationMax(int newValue)
	{
		m_nSHigh = newValue;
		cv::setTrackbarPos("Sat Max", "Filtered Video", newValue);
	}
	virtual void setValueMin(int newValue)
	{
		m_nVLow = newValue;
		cv::setTrackbarPos("Val Min", "Filtered Video", newValue);
	}
	virtual void setValueMax(int newValue)
	{
		m_nVHigh = newValue;
		cv::setTrackbarPos("Val Max", "Filtered Video", newValue);
	}
	virtual int getPVHueMin() const
	{
		return m_nHLow;
	}
	virtual int getPVHueMax() const
	{
		return m_nHHigh;
	}
	virtual int getSaturationMin() const
	{
		return m_nSLow;
	}
	virtual int getSaturationMax() const
	{
		return m_nSHigh;
	}
	virtual int getValueMin() const
	{
		return m_nVLow;
	}
	virtual int getValueMax() const
	{
		return m_nVHigh;
	}
	virtual ~imgProc()
	{

	}

	void SendObjectDetectionPacket(ObjectDetectionPacket packet){
		std::vector<uint8_t> bytesVec(89);
		std::span<uint8_t> bytes(bytesVec.data(), bytesVec.size());
		ObjectDetectionPacketStructType::Pack(bytes, packet);
		m_objectDetectionDataSocket.send_to(asio::buffer(bytes), m_objectDetectionEndpoint);
	}

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

	std::pair<double, double> calculate_distance_and_angle(const std::vector<cv::Point>& contour, double focal_length, double actual_diameter, const cv::Point2d& camera_center)
	{
		if (contour.size() < 5) {
			return { -1.0, -1.0 }; // Indicate error with -1.0
		}

		cv::RotatedRect rect = cv::fitEllipse(contour);
		double apparent_diameter = std::min(rect.size.width, rect.size.height);
		double distance = (actual_diameter * focal_length) / apparent_diameter;

		double dx = rect.center.x - camera_center.x;
		double angle = std::atan2(dx, focal_length);
		m_nLastCoralDistanceTopic.SetDefault(distance);
		m_nLastCoralAngleTopic.SetDefault(angle);
		return { distance, angle };
	}

	std::pair<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>> fit_cylinder_contour(const std::vector<cv::Point>& contour)
	{
		if (contour.size() < 5) {
			return { {}, {} };
		}

		cv::RotatedRect ellipse = cv::fitEllipse(contour);
		cv::Point2f center = ellipse.center;
		float majorAxisLength = std::max(ellipse.size.width, ellipse.size.height);
		float minorAxisLength = std::min(ellipse.size.width, ellipse.size.height);
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

		std::vector<cv::Point> hull;
		cv::convexHull(contour, hull);

		return { hull, { majorAxisEndPoint1, majorAxisEndPoint2 } };
	}

	virtual void ProcessFrame( int nCam, CameraFrame const *pcf )
	{
		printf( "Have Frame from %d:%d %dx%d\n", nCam, pcf->u32Sequence(), pcf->nRows(), pcf->nCols() );
		cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image() );
#if (IMGPROC_GUI)
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

        if (!contours.empty()) {
            // Find the largest contour (assuming it's the cylinder)
            auto largest_contour = std::max_element(contours.begin(), contours.end(),
                [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                    return cv::contourArea(a) < cv::contourArea(b);
                });

            // Fit a contour and get endpoints
            auto [fitted_contour, endpoints] = fit_cylinder_contour(*largest_contour);

            if (!fitted_contour.empty()) {
                // Calculate distance and angle
                auto [distance, angle] = calculate_distance_and_angle(*largest_contour, m_focalLen, m_fCDiam, CAMERA_CENTER);

#if (IMGPROC_GUI)
                // Draw the fitted contour
                cv::drawContours(filtered_frame, std::vector<std::vector<cv::Point>>{fitted_contour}, -1, cv::Scalar(0, 255, 0), 2);

                // Draw the major axis line if endpoints are valid
                if (endpoints.first.x != 0 || endpoints.first.y != 0 || endpoints.second.x != 0 || endpoints.second.y != 0) {
                    cv::line(filtered_frame, endpoints.first, endpoints.second, cv::Scalar(255, 0, 0), 2);
                }

                // Display distance and angle (only if valid values were calculated)
                if (distance != -1.0 && angle != -1.0) {
                    std::string dist_text = "Distance: " + std::to_string(distance) + "m";
                    std::string angle_text = "Angle: " + std::to_string(angle) + "rad";
                    cv::putText(filtered_frame, dist_text, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                    cv::putText(filtered_frame, angle_text, cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                }
#endif
				printf( "%.3f   %.3f\n", distance, angle );
				struct timespec tsCur;
				struct timeval tvCur;
				struct timeval tvDelta;
	
				// Get current time since power on
				clock_gettime(CLOCK_MONOTONIC, &tsCur );
				TIMESPEC_TO_TIMEVAL( &tvCur, &tsCur );
	
				// Calculate delta between when frame was catured and now.
				timersub( &tvCur, pcf->ts(), &tvDelta );
				ObjectDetectionPacket packet = ObjectDetectionPacket(PACKET_KEY, PACKET_VERSION,nt::Now() - tvDelta.tv_usec, 1, ObjectDetectionInfo(distance, angle, 20, ObjectDetectionTypes.CORAL), ObjectDetectionInfo(), ObjectDetectionInfo(), ObjectDetectionInfo());
				SendObjectDetectionPacket(packet);
            }
        }


#if (IMGPROC_GUI)
		// Display the results
   		cv::imshow("Filtered Video", filtered_frame);
        cv::imshow("Mask", mask);

		if (cv::waitKey(1) == 'q') {
			cv::destroyAllWindows();
			exit(0);
		}
#endif
	}
};

