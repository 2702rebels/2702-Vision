#include <LogFrameType.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/imgcodecs.hpp>

namespace fs = std::filesystem;

void WriteNewLogHeader(std::ofstream &logfile, unsigned int width,
                       unsigned int height, LogFrameType logFrameType) {
  logfile.write((char *)&logFrameType, sizeof(LogFrameType));
  logfile.write((char *)&width, sizeof(width));
  logfile.write((char *)&height, sizeof(height));
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Format: " << argv[0] << " /oldlogdir /newlogdir" << std::endl;
    return 1;
  }

  fs::path oldLogDirectory(argv[1]);
  fs::path newLogDirectory(argv[2]);

  if (oldLogDirectory == newLogDirectory) {
    std::cerr << "Old log directory cannot be the same as the new log directory"
              << std::endl;
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

  for (const fs::directory_entry &entry :
       fs::directory_iterator(oldLogDirectory)) {
    if (!fs::is_regular_file(entry)) {
      continue;
    }
    if (entry.path().extension() != ".log") {
      continue;
    }
    std::cout << "Converting log file at: " << entry.path() << std::endl;
    cv::Mat frame;
    std::ifstream curLogFile(entry.path(), std::ios::in | std::ios::binary);
    std::ofstream outputLogFile(newLogDirectory / entry.path().filename(),
                                std::ios::out | std::ios::trunc |
                                    std::ios::binary);
    std::string frameType;
    bool haveWrittenHeader = false;
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
        char *frameData = new char[width * height * 3];
        if (!curLogFile.read(frameData, width * height * 3)) {
          std::cerr << "Failed to read frame data of RGB frame" << std::endl;
          continue;
        }
        if (!haveWrittenHeader) {
          WriteNewLogHeader(outputLogFile, width, height, LogFrameType::RGB);
          haveWrittenHeader = true;
        }
        outputLogFile.write(frameData, width * height * 3);
        delete[] frameData;
      } else if (frameType == "<<FRM>>") {
        std::string tmp;
        // discard u32Seq and u32Exp
        std::getline(curLogFile, tmp);
        std::getline(curLogFile, tmp);
        // get height and width of frame
        int height, width;
        std::getline(curLogFile, tmp);
        height = atoi(tmp.c_str());
        std::getline(curLogFile, tmp);
        width = atoi(tmp.c_str());
        char *frameData = new char[width * height];
        if (!curLogFile.read(frameData, width * height)) {
          std::cerr << "Failed to read frame data of grayscale frame"
                    << std::endl;
          continue;
        }
        if (!haveWrittenHeader) {
          WriteNewLogHeader(outputLogFile, width, height,
                            LogFrameType::GRAYSCALE);
          haveWrittenHeader = true;
        }
        outputLogFile.write(frameData, width * height);
        delete[] frameData;
      } else if (frameType == "<<FRMPNG>>") {
        std::string tmp;
        // discard u32Seq and u32Exp
        std::getline(curLogFile, tmp);
        std::getline(curLogFile, tmp);
        int dataSize;
        std::getline(curLogFile, tmp);
        dataSize = atoi(tmp.c_str());
        char *pngBuf = new char[dataSize];
        if (!curLogFile.read(pngBuf, dataSize)) {
          std::cerr << "Failed to read frame data of PNG frame" << std::endl;
          continue;
        }
        cv::Mat pngImage;
        pngImage = cv::imdecode(cv::Mat(1, dataSize, CV_8UC1, pngBuf),
                                cv::IMREAD_UNCHANGED);
        if (!haveWrittenHeader) {
          WriteNewLogHeader(outputLogFile, pngImage.cols, pngImage.rows,
                            LogFrameType::GRAYSCALE);
          haveWrittenHeader = true;
        }
        int newFrameDataSize = pngImage.dataend - pngImage.data;
        outputLogFile.write((char *)pngImage.data, newFrameDataSize);
        delete[] pngBuf;
      } else {
        std::cerr << "Unrecognized frame type" << std::endl;
        continue;
      }
    }
    std::cout << "Translated log file at " << entry.path()
              << " to new log file at "
              << newLogDirectory / entry.path().filename() << std::endl;
    continue;
  }
  std::cout << "Translated all log files successfully" << std::endl;
  return 0;
}