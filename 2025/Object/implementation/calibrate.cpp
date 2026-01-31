/*! \brief Program to get the calibration data for the camera.
Loads all png images from /data/calibration/ to create these calibration values.
* Depends on:
* - opencv
*/

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include "LoggingUtils.h"

namespace fs = std::filesystem;
std::queue<fs::path> images;
std::mutex mtxVectorLock;
std::mutex mtxQueueLock;
std::vector<std::vector<cv::Point2f>> imagePoints;
std::vector<std::vector<cv::Point3f>> objectPoints;
std::vector<cv::Point3f> obj;
int chessboardWidth;
int chessboardHeight;
float squareSize;
cv::Size chessboardPatternSize;
volatile int nImagesDone = 0;
cv::Size imageSize;
bool useSBMethod = false;

void CalibratorThread(std::stop_token token)
{
	while (!token.stop_requested())
	{
		std::unique_lock queueLock(mtxQueueLock);
		if (images.empty())
		{
			return;
		}
		fs::path imagePath = images.front();
		images.pop();
		queueLock.unlock();
		cv::Mat img = cv::imread(imagePath.string(), cv::IMREAD_GRAYSCALE);
		imageSize = img.size();
		std::vector<cv::Point2f> corners;
		bool foundChessboard;
		if (useSBMethod == false)
		{
			foundChessboard = cv::findChessboardCorners(img, chessboardPatternSize, corners,
															 cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
		} else
		{
			foundChessboard = cv::findChessboardCornersSB(img, chessboardPatternSize, corners, cv::CALIB_CB_EXHAUSTIVE | cv::CALIB_CB_ACCURACY);
		}
		if (!foundChessboard)
		{
			std::cout << "WARN: Couldn't find chessboard for image" << imagePath.string() << std::endl;
			continue;
		}
		cv::cornerSubPix(img, corners, cv::Size(11, 11), cv::Size(-1, -1),
		                 cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1));
		std::unique_lock vectorLock(mtxVectorLock);
		imagePoints.push_back(corners);
		objectPoints.push_back(obj);
		vectorLock.unlock();
		nImagesDone++;
		info("{}% done", static_cast<double>(nImagesDone) / static_cast<double>(images.size()) * 100);
	}
}

int main(int argc, char** argv)
{
	SetupTextLogger();
	std::cout << "Enter chessboard width: ";
	std::cin >> chessboardWidth;
	std::cout << "Enter chessboard height: ";
	std::cin >> chessboardHeight;
	chessboardPatternSize = {chessboardWidth - 1, chessboardHeight - 1};
	std::cout << "Enter square size in m: ";
	std::cin >> squareSize;
	std::cout << "Use regular corners method or SB method? (enter r or s): ";
	std::string cornersMethod;
	std::cin >> cornersMethod;
	if (cornersMethod == "r")
	{
		useSBMethod = false;
	} else if (cornersMethod == "s")
	{
		useSBMethod = true;
	} else
	{
		std::cerr << "Failed to recognize corners method \"" << cornersMethod << "\"" << std::endl;
		return -1;
	}
	while (true)
	{
		std::string imagesDir;
		std::cout << "Enter a directory to add images from(null to finish): ";
		std::cin >> imagesDir;
		if (imagesDir == "null")
		{
			break;
		}
		for (auto item : fs::recursive_directory_iterator(imagesDir))
		{
			if (item.is_regular_file())
			{
				images.push(item.path());
			}
		}
	}
	for (int i = 0; i < chessboardPatternSize.height; i++)
	{
		for (int j = 0; j < chessboardPatternSize.width; j++)
		{
			obj.emplace_back(j * squareSize, i * squareSize, 0.0f);
		}
	}
	std::vector<std::jthread> calibratorThreads;
	int threadCount;
	std::cout << "Enter # of threads to use: ";
	std::cin >> threadCount;
	for (int i = 0; i < threadCount; i++)
	{
		calibratorThreads.emplace_back(CalibratorThread);
	}
	info("Starting finding of chessboards");
	for (std::jthread& thread : calibratorThreads)
	{
		thread.join();
	}
	info("Finished finding chessboards, now finding camera parameters");
	cv::Mat cameraMatrix, distCoeffs;
	std::vector<cv::Mat> rvecs, tvecs;
	double rms = cv::calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs);
	info("rms error: {}", rms);
	std::cout << "Camera matrix:\n" << cameraMatrix << std::endl;
	std::cout << "Distortion coefficients:\n" << distCoeffs << std::endl;
	return 0;
}
