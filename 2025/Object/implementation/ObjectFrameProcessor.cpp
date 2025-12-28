#include "ObjectFrameProcessor.h"
#include <numbers>
#include <cameraserver/CameraServer.h>
#include <spdlog/spdlog.h>
#include <functional>
#include <filesystem>
#include <opencv2/highgui.hpp>
#include <chrono>

namespace fs = std::filesystem;

// instantiate static var
template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
int64_t ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                             UseOutputFiltering>::m_i64TimeOffset;

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                     UseOutputFiltering>::ObjectFrameProcessor(int frameWidth, int frameHeight) :
	m_config(std::string(CONFIG_DIRECTORY) + "/processorConfig.json", ObjectDetectionSettings(), "ProcessorSettings"),
	m_frameWidth(frameWidth),
	m_frameHeight(frameHeight),
	CAMERA_CENTER(mtx.at<float>(0, 2), mtx.at<float>(1, 2)),
	m_p3dCam2Robot(m_config.Get().cameraoffsetx(), m_config.Get().cameraoffsety(), m_config.Get().cameraheight()),
	m_distanceFilter(m_config.Get().filterpacketwindow()),
	m_angleFilter(m_config.Get().filterpacketwindow()),
	udpSender(OBJECT_DETECTION_SERVER_IP, OBJECT_DETECTION_SERVER_PORT)
{
	m_config.LoadConfigFromFile();
	if constexpr (UseGui == ObjUseGui::ON)
	{
		cv::namedWindow("Filtered Video");

		cv::createTrackbar("Hue Min", "Filtered Video", nullptr, m_config.Get().huemin(), [](int pos, void* data)
		{
			auto* cfg = static_cast<ObjectDetectionConfig<ObjectDetectionSettings>*>(data);
			ObjectDetectionSettings newSettings;
			newSettings.CopyFrom(cfg->Get());
			newSettings.set_huemin(pos);
			cfg->LoadConfigFromStructure(newSettings);
		}, &m_config);
		cv::setTrackbarPos("Hue Min", "Filtered Video", m_config.Get().huemin());
		cv::createTrackbar("Hue Max", "Filtered Video", nullptr, m_config.Get().huemin(), [](int pos, void* data)
		{
			auto* cfg = static_cast<ObjectDetectionConfig<ObjectDetectionSettings>*>(data);
			ObjectDetectionSettings newSettings;
			newSettings.CopyFrom(cfg->Get());
			newSettings.set_huemax(pos);
			cfg->LoadConfigFromStructure(newSettings);
		}, &m_config);
		cv::setTrackbarPos("Hue Max", "Filtered Video", m_config.Get().huemax());
		cv::createTrackbar("Saturation Min", "Filtered Video", nullptr, m_config.Get().huemin(), [](int pos, void* data)
		{
			auto* cfg = static_cast<ObjectDetectionConfig<ObjectDetectionSettings>*>(data);
			ObjectDetectionSettings newSettings;
			newSettings.CopyFrom(cfg->Get());
			newSettings.set_saturationmin(pos);
			cfg->LoadConfigFromStructure(newSettings);
		}, &m_config);
		cv::setTrackbarPos("Saturation Min", "Filtered Video", m_config.Get().saturationmin());
		cv::createTrackbar("Saturation Max", "Filtered Video", nullptr, m_config.Get().huemin(), [](int pos, void* data)
		{
			auto* cfg = static_cast<ObjectDetectionConfig<ObjectDetectionSettings>*>(data);
			ObjectDetectionSettings newSettings;
			newSettings.CopyFrom(cfg->Get());
			newSettings.set_saturationmax(pos);
			cfg->LoadConfigFromStructure(newSettings);
		}, &m_config);
		cv::setTrackbarPos("Saturation Max", "Filtered Video", m_config.Get().saturationmax());
		cv::createTrackbar("Value Min", "Filtered Video", nullptr, m_config.Get().huemin(), [](int pos, void* data)
		{
			auto* cfg = static_cast<ObjectDetectionConfig<ObjectDetectionSettings>*>(data);
			ObjectDetectionSettings newSettings;
			newSettings.CopyFrom(cfg->Get());
			newSettings.set_valuemin(pos);
			cfg->LoadConfigFromStructure(newSettings);
		}, &m_config);
		cv::setTrackbarPos("Value Min", "Filtered Video", m_config.Get().valuemin());
		cv::createTrackbar("Value Max", "Filtered Video", nullptr, m_config.Get().huemin(), [](int pos, void* data)
		{
			auto* cfg = static_cast<ObjectDetectionConfig<ObjectDetectionSettings>*>(data);
			ObjectDetectionSettings newSettings;
			newSettings.CopyFrom(cfg->Get());
			newSettings.set_valuemax(pos);
			cfg->LoadConfigFromStructure(newSettings);
		}, &m_config);
		cv::setTrackbarPos("Value Max", "Filtered Video", m_config.Get().valuemax());

		m_config.AttachSetterFunction<int32_t>("hueMin", [](const int32_t& val)
		{
			cv::setTrackbarPos("Hue Min", "Filtered Video", val);
		});
		m_config.AttachSetterFunction<int32_t>("hueMax", [](const int32_t& val)
		{
			cv::setTrackbarPos("Hue Max", "Filtered Video", val);
		});
		m_config.AttachSetterFunction<int32_t>("saturationMin", [](const int32_t& val)
		{
			cv::setTrackbarPos("Saturation Min", "Filtered Video", val);
		});
		m_config.AttachSetterFunction<int32_t>("saturationMax", [](const int32_t& val)
		{
			cv::setTrackbarPos("Saturation Max", "Filtered Video", val);
		});
		m_config.AttachSetterFunction<int32_t>("valueMin", [](const int32_t& val)
		{
			cv::setTrackbarPos("Value Min", "Filtered Video", val);
		});
		m_config.AttachSetterFunction<int32_t>("valueMax", [](const int32_t& val)
		{
			cv::setTrackbarPos("Value Max", "Filtered Video", val);
		});
	}

	m_focalLen = (mtx.at<float>(0, 0) + mtx.at<float>(1, 1)) / 2;

	spdlog::info("Camera Matrix: {} {} {}\n{} {} {}\n{} {} {}",
	             mtx.at<float>(0, 0), mtx.at<float>(0, 1), mtx.at<float>(0, 2),
	             mtx.at<float>(1, 0), mtx.at<float>(1, 1), mtx.at<float>(1, 2),
	             mtx.at<float>(2, 0), mtx.at<float>(2, 1), mtx.at<float>(2, 2));
	spdlog::info("Focal Length Coefficients: {}, {}", mtx.at<float>(0, 0), mtx.at<float>(1, 2));
	spdlog::info("Focal Length: {}", m_focalLen);
	spdlog::info("Reached end of FrameProcessor constructor");
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
void ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                          UseOutputFiltering>::vStartLogging()
{
	fs::path logDir(m_config.Get().logdirectory());
	int nWriterThreads = m_config.Get().nlogwriterthreads();
	for (int i = 0; ; i++)
	{
		fs::path unfilteredPath(logDir / ("object-" + std::to_string(i) + "_unfiltered.log"));
		if (fs::exists(unfilteredPath))
		{
			continue;
		}
		m_unfilteredLogger = std::make_unique<AsyncVisionLog>(unfilteredPath, m_frameWidth, m_frameHeight,
		                                                      LogFrameType::RGB, nWriterThreads);
		break;
	}
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                     UseOutputFiltering>::~ObjectFrameProcessor() = default;

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
void ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                          UseOutputFiltering>::timesynccallback(nt::Event const& e)
{
	if (e.flags & NT_EVENT_TIMESYNC)
	{
		spdlog::info("tscb::: Have TIMESYNC\n");
		nt::TimeSyncEventData const* ptsed = e.GetTimeSyncEventData();
		m_i64TimeOffset = ptsed->serverTimeOffset;
		spdlog::info("Got Offset -> {}", ptsed->serverTimeOffset);
	}
	else
	{
		spdlog::info("tscb::: -> {}", e.flags);
	}
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
void ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                          UseOutputFiltering>::SendObjectDetectionPacket(const ObjectDetectionPacket& packet)
{
	if constexpr (SendPackets == ObjSendPackets::ON)
	{
		try
		{
			std::vector<uint8_t> bytesVec(StructTypeObjectDetectionPacket::GetSize());
			std::span<uint8_t> bytes(bytesVec.data(), bytesVec.size());
			StructTypeObjectDetectionPacket::Pack(bytes, packet);
			udpSender.bSend(bytes);
		}
		catch (std::exception& e)
		{
			spdlog::error("Failed to send packet with an exception: {}", e.what());
		}
	}
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
cv::Mat ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                             UseOutputFiltering>::filter_hsv_inverted(const cv::Mat& frame, int pv_hue_min,
                                                                      int pv_hue_max, int saturation_min,
                                                                      int saturation_max, int value_min, int value_max)
{
	cv::Mat hsv;
	cv::Mat mask;
	cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV); // heavy hitter, 5.99% of cpu time

	auto invert_and_scale_hue = [](int pv_hue)
	{
		int inverted_hue = (pv_hue != 0) ? 360 - pv_hue : 0;
		return inverted_hue / 2;
	};

	int cv_hue_min = invert_and_scale_hue(pv_hue_min);
	int cv_hue_max = invert_and_scale_hue(pv_hue_max);

	if (cv_hue_min > cv_hue_max)
	{
		std::swap(cv_hue_min, cv_hue_max);
	}

	cv::Scalar lower_bound(cv_hue_min, saturation_min, value_min);
	cv::Scalar upper_bound(cv_hue_max, saturation_max, value_max);
	cv::inRange(hsv, lower_bound, upper_bound, mask); // heavy hitter, 6.45% of cpu time
	return mask;
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
Rebels::Point2d ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                                     UseOutputFiltering>::calculate_coral_vector(
	const std::vector<cv::Point>& contour, double focal_length, double actual_diameter,
	const cv::Point2d& camera_center)
{
	cv::RotatedRect rect = cv::fitEllipse(contour);
	// Calculate the vector from the sensor to the object
	Rebels::Point3d v(1, -(rect.center.x - mtx.at<float>(0, 2)) / mtx.at<float>(0, 0),
	                  -(rect.center.y - mtx.at<float>(1, 2)) / mtx.at<float>(1, 1));

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

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
std::array<cv::Point2f, 4> ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection,
                                                SendPackets, UseOutputFiltering>::OrderPointArray(cv::Point2f* points)
{
	int highestIndex = 0;
	int max = 0;
	int lowestIndex = 0;
	int min = std::numeric_limits<int32_t>::max();
	// Find top-right point
	for (int i = 0; i < 4; i++)
	{
		if (points[i].x + points[i].y > max)
		{
			highestIndex = i;
			max = points[i].x + points[i].y;
		}
		if (points[i].x + points[i].y < min)
		{
			lowestIndex = i;
			min = points[i].x + points[i].y;
		}
	}
	std::array<cv::Point2f, 4> pointsSorted;
	pointsSorted[1] = points[highestIndex];
	pointsSorted[3] = points[lowestIndex];
	std::vector<int32_t> indexes = {0, 1, 2, 3};
	indexes.erase(std::remove(indexes.begin(), indexes.end(), highestIndex), indexes.end());
	indexes.erase(std::remove(indexes.begin(), indexes.end(), lowestIndex), indexes.end());
	if (points[indexes[0]].x > points[indexes[1]].x)
	{
		pointsSorted[2] = points[indexes[0]];
		pointsSorted[0] = points[indexes[1]];
	}
	else
	{
		pointsSorted[2] = points[indexes[1]];
		pointsSorted[0] = points[indexes[0]];
	}
	return pointsSorted;
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
std::tuple<std::vector<cv::Point>, std::pair<cv::Point, cv::Point>, cv::Point> ObjectFrameProcessor<
	LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
	UseOutputFiltering>::fit_cylinder_contour(const std::vector<cv::Point>& contour, cv::Mat mask)
{
	if (contour.size() < 5)
	{
		return {{}, {}, {}};
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
		return {{}, {}, {}};
	}
	float minorAxisLength = std::min(rect.width, rect.height);
	if (minorAxisLength < m_config.Get().minpixelheight() || minorAxisLength > m_config.Get().maxpixelheight())
	{
		return {{}, {}, {}};
	}
	double area = rect.width * rect.height;
	if (area < m_config.Get().minpixelarea() || area > m_config.Get().maxpixelarea())
	{
		return {{}, {}, {}};
	}
	// suspicious... the lines are coming out wrong the points must be goofed
	Rebels::LinearSlope line1(pointsSorted[0].x, pointsSorted[0].y, pointsSorted[1].x, pointsSorted[1].y); // top
	Rebels::LinearSlope line2(pointsSorted[1].x, pointsSorted[1].y, pointsSorted[2].x, pointsSorted[2].y); // right
	Rebels::LinearSlope line3(pointsSorted[2].x, pointsSorted[2].y, pointsSorted[3].x, pointsSorted[3].y); // bot
	Rebels::LinearSlope line4(pointsSorted[3].x, pointsSorted[3].y, pointsSorted[0].x, pointsSorted[0].y); // left
	int pixels = 0;
	int validPixels = 0;
	int colMin = std::max(rect.x, 0);
	int rowMin = std::max(rect.y, 0);
	int colMax = std::min(rect.width + rect.x, m_frameWidth - 1);
	int rowMax = std::min(rect.height + rect.y, m_frameHeight - 1);
	if (colMin > colMax)
	{
		int buf = colMin;
		colMin = colMax;
		colMax = buf;
	}
	if (rowMin > rowMax)
	{
		int buf = rowMin;
		rowMin = rowMax;
		rowMax = buf;
	}
	for (int col = colMin; col < colMax; col++)
	{
		for (int row = rowMin; row < rowMax; row++)
		{
			if (line1.IsPointBelowLine(col, row) && line2.IsPointBelowLine(col, row) && line3.IsPointAboveLine(col, row) && line4.
				IsPointBelowLine(col, row))
			{
				if (mask.at<uchar>(row, col) > 0)
				{
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
	if (detectpercent < m_config.Get().minpercentfill() || std::isnan(detectpercent))
	{
		return {{}, {}, {}};
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

	return {hull, {majorAxisEndPoint1, majorAxisEndPoint2}, ellipse.center};
}

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
void ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                          UseOutputFiltering>::ProcessFrame(int nCam, CameraFrame const* pcf)
{
	ProcessFrameWithRet(nCam, pcf);
}

struct EmptyStruct
{
};

template <ObjLatencyReporting LatencyReporting, ObjVideoDebuggingInfo VideoDebugging, ObjUseGui UseGui,
          ObjMultipleObjectDetection MultipleObjectDetection, ObjSendPackets SendPackets, ObjUseOutputFiltering
          UseOutputFiltering>
cv::Mat ObjectFrameProcessor<LatencyReporting, VideoDebugging, UseGui, MultipleObjectDetection, SendPackets,
                             UseOutputFiltering>::ProcessFrameWithRet(int nCam, CameraFrame const* pcf)
{
	spdlog::info("Have Frame from cam {}: {}x{}", nCam, pcf->nRows(), pcf->nCols());
	cv::Mat frame(pcf->nRows(), pcf->nCols(), CV_8UC3, (void*)pcf->pu8Image());
	if (m_unfilteredLogger)
	{
		m_unfilteredLogger->AddFrameToQueueAsync(frame);
	}
	if constexpr (UseGui == ObjUseGui::ON)
	{
		cv::imshow("Raw", frame);
	}
	cv::Mat mask = filter_hsv_inverted(frame, m_config.Get().huemin(), m_config.Get().huemax(),
	                                   m_config.Get().saturationmin(), m_config.Get().saturationmax(),
	                                   m_config.Get().valuemin(), m_config.Get().valuemax());
	std::conditional_t<VideoDebugging == ObjVideoDebuggingInfo::ON, cv::Mat, EmptyStruct> filtered_frame;
	// A little ugly, but the idea is that anything trying to use filtered_frame when video debugging is not enabled will fail
	std::conditional_t<VideoDebugging == ObjVideoDebuggingInfo::ON, int, EmptyStruct> heightPaintAt;
	if constexpr (VideoDebugging == ObjVideoDebuggingInfo::ON)
	{
		filtered_frame = cv::Mat::zeros(pcf->nRows(), pcf->nCols(), CV_8UC3);
		cv::bitwise_and(frame, frame, filtered_frame, mask); // BIG STALL, 9.3% of cpu time spent here
		heightPaintAt = 60;
	}
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	std::vector<ObjectDetectionInfo> corals;
	if (!contours.empty())
	{
		for (int i = 0; i < contours.size(); i++)
		{
			// Fit a contour and get endpoints
			if (MultipleObjectDetection == ObjMultipleObjectDetection::OFF)
			{
				auto largest_contour = std::max_element(contours.begin(), contours.end(),
				                                        [](const std::vector<cv::Point>& a,
				                                           const std::vector<cv::Point>& b)
				                                        {
					                                        return cv::contourArea(a) < cv::contourArea(b);
				                                        });
				contours[i] = *largest_contour;
			}
			auto [fitted_contour, endpoints, center] = fit_cylinder_contour(contours[i], mask);
			if (!fitted_contour.empty())
			{
				if constexpr (VideoDebugging == ObjVideoDebuggingInfo::ON)
				{
					std::string uvtext = "(" + std::to_string(center.x) + ", " + std::to_string(center.y) + ")";
					cv::putText(filtered_frame, uvtext, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1,
								cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
					heightPaintAt += 30;
				}
				Rebels::Point2d coralVec = calculate_coral_vector(contours[i], m_focalLen, m_config.Get().fcdiam(),
				                                                  CAMERA_CENTER);
				double distance = coralVec.length();
				// u = x, v = y
				double angle = std::atan2(coralVec.v(), coralVec.u());
				spdlog::info("distance: {}, angle: {}", distance, angle);
				// NaN compare operations are always false, so if either is NaN, don't use it.
				if (distance == distance && angle == angle)
				{
					if constexpr (UseOutputFiltering == ObjUseOutputFiltering::ON)
					{
						distance = m_distanceFilter.Calculate(distance);
						angle = m_angleFilter.Calculate(angle);
					}
					m_nLastCoralDistanceTopic.SetDefault(distance);
					m_nLastCoralAngleTopic.SetDefault(angle);
					corals.push_back(ObjectDetectionInfo(distance, angle, (uint8_t)20, ObjectDetectionType::CORAL));
					// Draw the fitted contour
					if constexpr (VideoDebugging == ObjVideoDebuggingInfo::ON)
					{
						std::string coral_vec_text = "(" + std::to_string(coralVec.u()) + ", " + std::to_string(
							coralVec.v()) + ")";
						std::string distance_text = "Distance: " + std::to_string(distance / 0.0254) + "in";
						std::string angle_text = "Angle: " + std::to_string(angle * (180 / std::numbers::pi)) + "deg";
						cv::putText(filtered_frame, coral_vec_text, cv::Point(10, heightPaintAt),
						            cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
						heightPaintAt += 30;
						cv::putText(filtered_frame, distance_text, cv::Point(10, heightPaintAt),
						            cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
						heightPaintAt += 30;
						cv::putText(filtered_frame, angle_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX,
						            1, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
						heightPaintAt += 30;
						cv::drawContours(filtered_frame, std::vector<std::vector<cv::Point>>{fitted_contour}, -1,
						                 cv::Scalar(0, 255, 0), 2);
						// Draw the major axis line if endpoints are valid
						if (endpoints.first.x != 0 || endpoints.first.y != 0 || endpoints.second.x != 0 || endpoints.
							second.y != 0)
						{
							cv::line(filtered_frame, endpoints.first, endpoints.second, cv::Scalar(255, 0, 0), 2);
						}
					}
				}
			}
			if constexpr (MultipleObjectDetection == ObjMultipleObjectDetection::OFF)
			{
				break;
			}
		}
		if constexpr (SendPackets == ObjSendPackets::ON)
		{
			// Get current time since power on
			std::chrono::system_clock::time_point tsCur = std::chrono::system_clock::now();
			// Calculate delta between when frame was catured and now.
			std::chrono::system_clock::duration frameProcDelta = tsCur - *pcf->ts();
			std::sort(corals.begin(), corals.end());
			int count = corals.size();
			while (corals.size() < 4)
			{
				corals.push_back(ObjectDetectionInfo());
			}
			long time = nt::Now() + m_i64TimeOffset - std::chrono::duration_cast<
				std::chrono::microseconds>(frameProcDelta).count();
			// timestamp is at frame capture time in microseconds with NetworkTables server-adjusted time
			ObjectDetectionPacket packet(PACKET_KEY, PACKET_VERSION, time, count, corals.at(0), corals.at(1),
			                             corals.at(2), corals.at(3));
			SendObjectDetectionPacket(packet);
		}
	}

	if constexpr (LatencyReporting == ObjLatencyReporting::ON)
	{
		std::chrono::system_clock::time_point tsCur = std::chrono::system_clock::now();
		// Calculate delta between when frame was catured and now.
		std::chrono::system_clock::duration frameProcDelta = tsCur - *pcf->ts();
		m_nLastLatencyTopic.SetDefault(std::chrono::duration_cast<std::chrono::microseconds>(frameProcDelta).count());

		if constexpr (VideoDebugging == ObjVideoDebuggingInfo::ON)
		{
			// Display distance and angle (only if valid values were calculated)
			std::string latency_text = "Latency: " + std::to_string(
				std::chrono::duration_cast<std::chrono::microseconds>(frameProcDelta).count()) + "us";
			cv::putText(filtered_frame, latency_text, cv::Point(10, heightPaintAt), cv::FONT_HERSHEY_SIMPLEX, 1,
			            cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
			heightPaintAt += 30;
			if (m_mtxCS.try_lock())
			{
				if (m_bFirstVid)
				{
					m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows());
					m_outputStreamRaw = frc::CameraServer::PutVideo("VisionRaw", pcf->nCols(), pcf->nRows());
					m_bFirstVid = false;
				}
				m_outputStream.PutFrame(filtered_frame);
				m_outputStreamRaw.PutFrame(frame);
				m_mtxCS.unlock();
			}
		}
	}


	if constexpr (UseGui == ObjUseGui::ON)
	{
		// Display the results
		if constexpr (VideoDebugging == ObjVideoDebuggingInfo::ON)
		{
			cv::imshow("Filtered Video", filtered_frame);
		}
		cv::imshow("Mask", mask);

		if (cv::waitKey(1) == 'q')
		{
			cv::destroyAllWindows();
			exit(0);
		}
	}
	if constexpr (VideoDebugging == ObjVideoDebuggingInfo::ON)
	{
		return filtered_frame;
	}
	else
	{
		return {};
	}
}

template class ObjectFrameProcessor<ObjLatencyReporting::ON, ObjVideoDebuggingInfo::ON, ObjUseGui::OFF,
                                    ObjMultipleObjectDetection::OFF, ObjSendPackets::OFF, ObjUseOutputFiltering::ON>;
template class ObjectFrameProcessor<ObjLatencyReporting::ON, ObjVideoDebuggingInfo::ON, ObjUseGui::OFF,
                                    ObjMultipleObjectDetection::OFF, ObjSendPackets::ON, ObjUseOutputFiltering::ON>;
template class ObjectFrameProcessor<ObjLatencyReporting::ON, ObjVideoDebuggingInfo::ON, ObjUseGui::ON,
                                    ObjMultipleObjectDetection::OFF, ObjSendPackets::OFF, ObjUseOutputFiltering::ON>;
