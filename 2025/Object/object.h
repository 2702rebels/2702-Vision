#pragma once

class imgProc : public FrmProcessor
{
	uint8_t		m_u8HLow = 179;
	uint8_t		m_u8SLow = 0;
	uint8_t		m_u8VLow = 147;
	uint8_t		m_u8HHigh = 180;
	uint8_t		m_u8SHigh = 255;
	uint8_t		m_u8VHigh = 255;

	double		m_fCDiam = 0.1016;

	double		distance;
	
	cv::Mat mtx = (cv::Mat_<float>(3, 3) << 568.20791041, 0.0, 341.37830129,
												0.0, 568.20791041, 240.37830129,
												0.0, 0.0, 1.0);

	cv::Mat dist = (cv::Mat_<float>(1, 5) << 0.08705563, 0.10725078, -0.01064468, 0.01151696, -0.33419273);
	
	double		 m_focalLen = (mtx.at<double>(0, 0) + mtx.at<double>(1, 1)) / 2;
    cv::Point2d CAMERA_CENTER = (mtx.at<double>(0, 2), mtx.at<double>(1, 2));

public:
	imgProc()
	{
	}
	virtual ~imgProc()
	{

	}
	cv::Mat filter_hsv_inverted(const cv::Mat& frame, int pv_hue_min, int pv_hue_max, int saturation_min, int saturation_max, int value_min, int value_max) {
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

	std::pair<double, double> calculate_distance_and_angle(const std::vector<cv::Point>& contour, double focal_length, double actual_diameter, const cv::Point2d& camera_center) {
		if (contour.size() < 5) {
			return { -1.0, -1.0 }; // Indicate error with -1.0
		}

		cv::RotatedRect rect = cv::fitEllipse(contour);
		double apparent_diameter = std::min(rect.size.width, rect.size.height);
		double distance = (actual_diameter * focal_length) / apparent_diameter;

		double dx = rect.center.x - camera_center.x;
		double angle = std::atan2(dx, focal_length);
		double angle_degrees = angle * 180.0 / CV_PI;

		return { distance, angle_degrees };
	}

	std::pair<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>> fit_cylinder_contour(const std::vector<cv::Point>& contour) {
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
		// cv::Mat img( pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image() );
		// cv::Mat hsv( pcf->nRows(), pcf->nCols(), CV_8UC3);

		// cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

		// cv::Mat thresh;
		// cv::inRange(hsv, cv::Scalar( m_u8HLow, m_u8SLow, m_u8VLow), cv::Scalar( m_u8HHigh, m_u8SHigh, m_u8VHigh), thresh);

		#if 0

		cv::VideoCapture cap(1);
		if (!cap.isOpened()) {
			std::cerr << "Error opening camera!" << std::endl;
			return -1;
		}

		cv::namedWindow("Filtered Video");
		cv::createTrackbar("PV Hue Min", "Filtered Video", &pv_hue_min, 360);
		cv::createTrackbar("PV Hue Max", "Filtered Video", &pv_hue_max, 360);
		cv::createTrackbar("Sat Min", "Filtered Video", &saturation_min, 255);
		cv::createTrackbar("Sat Max", "Filtered Video", &saturation_max, 255);
		cv::createTrackbar("Val Min", "Filtered Video", &value_min, 255);
		cv::createTrackbar("Val Max", "Filtered Video", &value_max, 255);

		#endif

		frame = undistort_frame(frame, mtx, dist);

        #if 0

		// Get current trackbar positions
        pv_hue_min = cv::getTrackbarPos("PV Hue Min", "Filtered Video");
        pv_hue_max = cv::getTrackbarPos("PV Hue Max", "Filtered Video");
        saturation_min = cv::getTrackbarPos("Sat Min", "Filtered Video");
        saturation_max = cv::getTrackbarPos("Sat Max", "Filtered Video");
        value_min = cv::getTrackbarPos("Val Min", "Filtered Video");
        value_max = cv::getTrackbarPos("Val Max", "Filtered Video");

		#endif

        cv::Mat mask = filter_hsv_inverted(frame, m_u8HLow, m_u8HHigh, m_u8SLow, m_u8SHigh, m_u8VLow, m_u8VHigh);
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

                // Draw the fitted contour
                cv::drawContours(filtered_frame, std::vector<std::vector<cv::Point>>{fitted_contour}, -1, cv::Scalar(0, 255, 0), 2);

                // Draw the major axis line if endpoints are valid
                if (endpoints.first.x != 0 || endpoints.first.y != 0 || endpoints.second.x != 0 || endpoints.second.y != 0) {
                    cv::line(filtered_frame, endpoints.first, endpoints.second, cv::Scalar(255, 0, 0), 2);
                }

                // Display distance and angle (only if valid values were calculated)
                if (distance != -1.0 && angle != -1.0) {
                    std::string dist_text = "Distance: " + std::to_string(distance) + "m";
                    std::string angle_text = "Angle: " + std::to_string(angle) + "deg";
                    cv::putText(filtered_frame, dist_text, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                    cv::putText(filtered_frame, angle_text, cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                }
            }
        }

		#if 0

		// Display the results
        cv::imshow("Filtered Video", filtered_frame);
        cv::imshow("Mask", mask);

		if (cv::waitKey(1) == 'q') {
            cap.release();
			cv::destroyAllWindows();
			return 0;

        }

		#endif

	}
};

