#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <opencv2/imgproc.hpp>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Format: " << argv[0] << " /oldlogdir /newlogdir" << std::endl;
		return 1;
	}

	std::path oldLogDirectory(argv[1]);
	std::path newLogDirectory(argv[2]);

	if (oldLogDirectory == newLogDirectory) {
		std::cerr << "Old log directory cannot be the same as the new log directory" << std::endl;
		return 1;
	}
	if (!fs::is_directory(oldLogDirectory)) {
		std::cerr << "Old log directory must be a path to a directory" << std::endl;
		return 1;
	}
	if (!fs::is_directory(newLogDirectory)) {
		std::cerr << "New log directory must be a path to a directory" << std::endl;
		return 1;
	}

	for (const fs::directory_entry& entry : fs::directory_iterator(oldLogDirectory)) {
		if (!fs::is_regular_file(entry)) {
			continue;
		}
		if (entry.path().extension() != ".log") {
			continue;
		}
		std::cout << "Converting log file at: " << entry.path() << std::endl;
		cv::Mat frame;
		std::ifstream curLogFile(entry.path());
		std::ofstream outputLogFile(newLogDirectory.path() / entry.filename());
		std::string frameType;
		while (std::getline(curLogFile, frameType)) {
			if (frameType == "<<FRMRGB>>") {
				std::string tmp;
				// discard u32Seq and u32Exp
				std::getline(curLogFile, tmp);
				std::getline(curLogFile, tmp);
				// get height and width of frame
				int height, width;
				std::getline(curLogFile, tmp);
				height = stoi(tmp);
				std::getline(curLogFile, tmp);
				width = stoi(tmp);
				char[width * height * 3] frameData;
				if (!curLogFile.read(frameData, width * height * 3)) {
					std::cerr << "Failed to read frame data of RGB frame" << std::endl;
					return 1;
				}
			}
			else if (frameType == "<<FRM>>") {
				std::string tmp;
				// discard u32Seq and u32Exp
				std::getline(curLogFile, tmp);
				std::getline(curLogFile, tmp);
				int height, width;
				std::getline(curLogFile, tmp);
				height = atoi(tmp);
				std::getline(curLogFile, tmp);
				width = atoi(tmp);
				char[width * height] framedata;
				if (!curLogFile.read(frameData, width * height)) {
					std::cerr << "Failed to read frame data of raw frame" << std::endl;
					return 1;
				}
			}
			else if (frameType == "<<FRMPNG>>") {
				std::string tmp;
				// discard u32Seq and u32Exp
				std::getline(curLogFile, tmp);
				std::getline(curLogFile, tmp);
				int dataSize;
				std::getline(curLogFile, tmp);
				dataSize = atoi(tmp);
				char pngBuf[dataSize];
				if (!curLogFile.read(pngBuf, dataSize)) {
					std::cerr << "Failed to read frame data of PNG frame" << std::endl;
					return 1;
				}
				cv::Mat pngImage;
				pngImage = cv::imdecode(cv::Mat(1, dataSize, CV_8UC1, pngBuf), cv::IMREAD_UNCHANGED);
			}
			else {
				std::cerr << "Unrecognized frame type" << std::endl;
				return 1;
			}
		}
		std::cout << "Finished reading log file" << std::endl;
		continue;
	}
}