#include "ObjectFrameProcessor.h"
#include <fstream>
#include <cameraserver/CameraServer.h>
#include <cstdint>
#include <spdlog/spdlog.h>

// Initialize static vars
int64_t ObjectFrameProcessor::m_i64TimeOffset;

ObjectFrameProcessor::ObjectFrameProcessor(int frameWidth, int frameHeight) :
	FrmProcessor(),
	m_config("./processorSettings.json", ObjectDetectionSettings(), "ProcessorSettings"),
	CAMERA_CENTER(mtx.at<float>(0, 2), mtx.at<float>(1, 2)),
	m_p3dCam2Robot(m_config.Get().cameraoffsetx(), m_config.Get().cameraoffsety(), m_config.Get().cameraheight()),
	m_distanceFilter(m_config.Get().filterpacketwindow()),
	m_angleFilter(m_config.Get().filterpacketwindow()),
	m_frameWidth(frameWidth),
	m_frameHeight(frameHeight)
{
	m_focalLen = (mtx.at<float>(0, 0) + mtx.at<float>(1, 1)) / 2;

	spdlog::info("Camera Matrix: {} {} {}\n{} {} {}\n{} {} {}",
		mtx.at<float>(0, 0), mtx.at<float>(0, 1), mtx.at<float>(0, 2),
		mtx.at<float>(1, 0), mtx.at<float>(1, 1), mtx.at<float>(1, 2),
		mtx.at<float>(2, 0), mtx.at<float>(2, 1), mtx.at<float>(2, 2));
	spdlog::info("Focal Length Coefficients: {}, {}", mtx.at<float>(0, 0), mtx.at<float>(1, 2));
	spdlog::info("Focal Length: {}", m_focalLen);
#if (DEBUG_VIDEO || IMGPROC_GUI) && !PRODUCTION
	m_angleFile.open("./angle.txt");
	m_distFile.open("./dist.txt");
	m_latencyFile.open("./latency.txt");
#endif
#if (IMGPROC_GUI && !PRODUCTION)
	cv::namedWindow("Filtered Video");

	cv::createTrackbar("PV Hue Min", "Filtered Video", config->Get().huemin(), 360);
	cv::createTrackbar("PV Hue Max", "Filtered Video", config->Get().huemax(), 360);
	cv::createTrackbar("Sat Min", "Filtered Video", config->Get().saturationmin(), 255);
	cv::createTrackbar("Sat Max", "Filtered Video", config->Get().saturationmax(), 255);
	cv::createTrackbar("Val Min", "Filtered Video", config->Get().valuemin(), 255);
	cv::createTrackbar("Val Max", "Filtered Video", config->Get().valuemax(), 255);
#endif
	spdlog::info("Reached end of FrameProcessor constructor");
}

void ObjectFrameProcessor::vStartLogging() {
	m_bIsLogging = true;
}

/*!
* \brief Sets hue min object detection parameter.
* \param newValue - integer representing the new hue min value
*/
void ObjectFrameProcessor::setPVHueMin(const int& newValue)
{
#if (IMGPROC_GUI && !PRODUCTION)
	cv::setTrackbarPos("PV Hue Min", "Filtered Video", newValue);
#endif
}
/*!
* \brief Sets hue max object detection parameter.
* \param newValue - integer representing the new hue max value
*/
void ObjectFrameProcessor::setPVHueMax(const int& newValue)
{
#if (IMGPROC_GUI && !PRODUCTION)
	cv::setTrackbarPos("PV Hue Max", "Filtered Video", newValue);
#endif
}
/*!
* \brief Sets saturation min object detection parameter.
* \param newValue - integer representing the new saturation min value
*/
void ObjectFrameProcessor::setSaturationMin(const int& newValue)
{
#if (IMGPROC_GUI && !PRODUCTION)
	cv::setTrackbarPos("Sat Min", "Filtered Video", newValue);
#endif
}
/*!
* \brief Sets saturation max object detection parameter.
* \param newValue - integer representing the new saturation max value
*/
void ObjectFrameProcessor::setSaturationMax(const int& newValue)
{
#if (IMGPROC_GUI && !PRODUCTION)
	cv::setTrackbarPos("Sat Max", "Filtered Video", newValue);
#endif
}
/*!
* \brief Sets value min object detection parameter.
* \param newValue - integer representing the new value min value
*/
void ObjectFrameProcessor::setValueMin(const int& newValue)
{
#if (IMGPROC_GUI && !PRODUCTION)
	cv::setTrackbarPos("Val Min", "Filtered Video", newValue);
#endif
}
/*!
* \brief Sets value max object detection parameter.
* \param newValue - integer representing the new value max value
*/
void ObjectFrameProcessor::setValueMax(const int& newValue)
{
#if (IMGPROC_GUI && !PRODUCTION)
	cv::setTrackbarPos("Val Max", "Filtered Video", newValue);
#endif
}

ObjectFrameProcessor::~ObjectFrameProcessor() {}

void ObjectFrameProcessor::timesynccallback(nt::Event const& e)
{
	if (e.flags & NT_EVENT_TIMESYNC) {
		spdlog::info("tscb::: Have TIMESYNC\n");
		nt::TimeSyncEventData const* ptsed = e.GetTimeSyncEventData();
		m_i64TimeOffset = ptsed->serverTimeOffset;
		spdlog::info("Got Offset -> {}", ptsed->serverTimeOffset);
	}
	else {
		spdlog::info("tscb::: -> {}", e.flags);
	}
}

void ObjectFrameProcessor::SendObjectDetectionPacket(ObjectDetectionPacket packet) {
#if SEND_PACKETS || PRODUCTION
	try {
		std::vector<uint8_t> bytesVec(StructTypeObjectDetectionPacket::GetSize());
		std::span<uint8_t> bytes(bytesVec.data(), bytesVec.size());
		StructTypeObjectDetectionPacket::Pack(bytes, packet);
		udpSender.bSend(bytes);
	}
	catch (std::exception& e) {
		spdlog::error("Failed to send packet with an exception: {}", e.what());
	}
#endif
}

cv::Mat ObjectFrameProcessor::filter_hsv_inverted(const cv::Mat& frame, int pv_hue_min, int pv_hue_max, int saturation_min, int saturation_max, int value_min, int value_max)
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

Rebels::Point2d ObjectFrameProcessor::calculate_coral_vector(const std::vector<cv::Point>& contour, double focal_length, double actual_diameter, const cv::Point2d& camera_center)
{
	cv::RotatedRect rect = cv::fitEllipse(contour);
	// Calculate the vector from the sensor to the object
	Rebels::Point3d v(1, -(rect.center.x - mtx.at<float>(0, 2)) / mtx.at<float>(0, 0), -(rect.center.y - mtx.at<float>(1, 2)) / mtx.at<float>(1, 1));

	// The Camera is tiled down, so rotate the vector
	// to "Robot Space"
	Rebels::Quaternion qCamTilt(m_config.Get().cameraangle(), Rebels::Point3d(0., 1., 0.));
	Rebels::Point3d vRot = qCamTilt * v;

	// Find the Z intercept -- the point where the vector touches the floor
	double fT = -m_p3dCam2Robot.z() / vRot.z();

	// Now compute the location of the point
	Rebels::Point3d pos = vRot * fT + m_p3dCam2Robot;
	return Rebels::Point2d(pos.x(), pos.y());
}

std::array<cv::Point2f, 4> ObjectFrameProcessor::OrderPointArray(cv::Point2f* points) {
	int highestIndex = 0;
	int max = 0;
	int lowestIndex = 0;
	int min = std::numeric_limits<int>::max();
	// Find top-right point
	for (int i = 0; i < 4; i++) {
		if (points[i].x + points[i].y > max) {
			highestIndex = i;
			max = points[i].x + points[i].y;
		}
		if (points[i].x + points[i].y < min) {
			lowestIndex = i;
			min = points[i].x + points[i].y;
		}
	}
	std::array<cv::Point2f, 4> pointsSorted;
	pointsSorted[1] = points[highestIndex];
	pointsSorted[3] = points[lowestIndex];
	std::vector<int> indexes = { 0,1,2,3 };
	indexes.erase(std::remove(indexes.begin(), indexes.end(), highestIndex), indexes.end());
	indexes.erase(std::remove(indexes.begin(), indexes.end(), lowestIndex), indexes.end());
	if (points[indexes[0]].x > points[indexes[1]].x) {
		pointsSorted[2] = points[indexes[0]];
		pointsSorted[0] = points[indexes[1]];
	}
	else {
		pointsSorted[2] = points[indexes[1]];
		pointsSorted[0] = points[indexes[0]];
	}
	return pointsSorted;
}

std::pair<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>> ObjectFrameProcessor::fit_cylinder_contour(const std::vector<cv::Point>& contour, cv::Mat mask, cv::Mat filteredFrame, int* heightPaintAt)
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
	if (majorAxisLength < m_config.Get().minpixelwidth() || majorAxisLength > m_config.Get().maxpixelwidth())
	{
		return { {}, {} };
	}
	float minorAxisLength = std::min(rect.width, rect.height);
	if (minorAxisLength < m_config.Get().minpixelheight() || minorAxisLength > m_config.Get().maxpixelheight()) {
		return { {}, {} };
	}
	double area = rect.width * rect.height;
	if (area < m_config.Get().minpixelarea() || area > m_config.Get().maxpixelarea()) {
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
	int xMax = std::min(rect.width + rect.x, m_frameWidth - 1);
	int yMax = std::min(rect.height + rect.y, m_frameHeight - 1);
	if (xMin > xMax) {
		int buf = xMin;
		xMin = xMax;
		xMax = buf;
	}
	if (yMin > yMax) {
		int buf = yMin;
		yMin = yMax;
		yMax = buf;
	}
	for (int x = xMin; x < xMax; x++) {
		for (int y = yMin; y < yMax; y++) {
			if (line1.IsPointBelowLine(x, y) && line2.IsPointBelowLine(x, y) && line3.IsPointAboveLine(x, y) && line4.IsPointBelowLine(x, y)) {
				if (mask.at<uchar>(x, y) > 0) {
					validPixels++;
				}
				pixels++;
			}
		}
	}
	double detectpercent = (double)validPixels / (double)pixels;
	spdlog::info("detectpercent: {}", detectpercent);
	spdlog::info("npixels: {}", pixels);
	spdlog::info("validpixels: {}", validPixels);
	// If % detection is nan or less than min fill
	if (detectpercent < m_config.Get().minpercentfill() || std::isnan(detectpercent)) {
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

void ObjectFrameProcessor::ProcessFrame(int nCam, CameraFrame const* pcf) {
	ProcessFrameWithRet(nCam, pcf);
}

cv::Mat ObjectFrameProcessor::ProcessFrameWithRet(int nCam, CameraFrame const* pcf)
{
	spdlog::info("Have Frame from cam {}:#{} {}x{}", nCam, pcf->u32Sequence(), pcf->nRows(), pcf->nCols());
	cv::Mat frame(pcf->nRows(), pcf->nCols(), CV_8UC3, (void*)pcf->pu8Image());
	int heightPaintAt = 60;
	if (m_mtxLog.try_lock()) {
		if (m_bLoggingDone) {
		}
		else if (m_vl.bIsOpen()) {
			m_vl.bWriteRGB(frame, pcf->u32Sequence(), pcf->u32Exposure());
			if (time(0) > m_tmLogEnd) {
				m_vl.vClose();
				m_bLoggingDone = true;
				spdlog::info("Finished logging");
			}
		}
		else {
			if (m_bIsLogging) {
				try {
					char szTmp[128];
					for (int i = 0; ; i++) {
						snprintf(szTmp, sizeof(szTmp), "%s/object-%04d.log", m_config.Get().logdirectory().c_str(), i);
						if (access(szTmp, F_OK) == -1) {
							break;
						}
					}
					spdlog::info("Opening log file at {}", szTmp);
					m_vl.vOpen(szTmp, true);
					m_tmLogEnd = time(0) + 160;
				}
				catch (std::exception& e) {
					m_bLoggingDone = true;
					spdlog::error("Unable to start logging: {}", e.what());
				}
			}
		}
		m_mtxLog.unlock();
	}
#if (IMGPROC_GUI && !PRODUCTION)
	cv::imshow("Raw", frame);
#endif
	// cv::Mat hsv( pcf->nRows(), pcf->nCols(), CV_8UC3);

	// cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

	// cv::Mat thresh;
	// cv::inRange(hsv, cv::Scalar( m_nHLow, m_nSLow, m_nVLow), cv::Scalar( m_nHHigh, m_nSHigh, m_nVHigh), thresh);

	//frame = cv::undistort_frame(frame, mtx, dist);
	cv::Mat mask = filter_hsv_inverted(frame, m_config.Get().huemin(), m_config.Get().huemax(), m_config.Get().saturationmin(), m_config.Get().saturationmax(), m_config.Get().valuemin(), m_config.Get().valuemax());
	cv::Mat filtered_frame;
	cv::bitwise_and(frame, frame, filtered_frame, mask);
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	std::vector<ObjectDetectionInfo> corals;
	if (!contours.empty()) {
		for (int i = 0; i < contours.size(); i++) {
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
				Rebels::Point2d coralVec = calculate_coral_vector(contours[i], m_focalLen, m_config.Get().fcdiam(), CAMERA_CENTER);
				double distance = coralVec.length();
				// u = x, v = y
				double angle = std::atan2(coralVec.v(), coralVec.u());
				spdlog::info("distance: {}, angle: {}", distance, angle);
				// NaN compare operations are always false, so if either is NaN, don't use it.
				if (distance == distance && angle == angle) {
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
					std::string angle_text = "Angle: " + std::to_string(angle * (180 / M_PI)) + "deg";
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
		clock_gettime(CLOCK_MONOTONIC_RAW, &tsCur);
		TIMESPEC_TO_TIMEVAL(&tvCur, &tsCur);
		// Calculate delta between when frame was catured and now.
		timersub(&tvCur, pcf->ts(), &tvDelta);
		std::sort(corals.begin(), corals.end());
		int count = corals.size();
		while (corals.size() < 4) {
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
			m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows());
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