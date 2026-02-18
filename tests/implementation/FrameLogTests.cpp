#include <VisionLog.h>
#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

TEST(FrameLog, GrayscaleHeader) {
  VisionLogWriter *writer = new VisionLogWriter(
      fs::path("./grayscaleHeader.log"), LogFrameType::GRAYSCALE, 250, 750);
  EXPECT_EQ(writer->GetLogType(), LogFrameType::GRAYSCALE);
  EXPECT_EQ(writer->GetWidth(), 250);
  EXPECT_EQ(writer->GetHeight(), 750);
  delete writer;
  VisionLogReader reader(fs::path("./grayscaleHeader.log"));
  EXPECT_EQ(reader.GetLogType(), LogFrameType::GRAYSCALE);
  EXPECT_EQ(reader.GetWidth(), 250);
  EXPECT_EQ(reader.GetHeight(), 750);
}

TEST(FrameLog, RGBHeader) {
  VisionLogWriter *writer = new VisionLogWriter(fs::path("./rgbHeader.log"),
                                                LogFrameType::RGB, 1920, 1080);
  EXPECT_EQ(writer->GetLogType(), LogFrameType::RGB);
  EXPECT_EQ(writer->GetWidth(), 1920);
  EXPECT_EQ(writer->GetHeight(), 1080);
  delete writer;
  VisionLogReader reader(fs::path("./rgbHeader.log"));
  EXPECT_EQ(reader.GetLogType(), LogFrameType::RGB);
  EXPECT_EQ(reader.GetWidth(), 1920);
  EXPECT_EQ(reader.GetHeight(), 1080);
}

TEST(FrameLog, RGBFrames) {
  int nFrames = 3;
  VisionLogWriter *writer = new VisionLogWriter(fs::path("./rgbFrames.log"),
                                                LogFrameType::RGB, 640, 480);
  std::vector<cv::Mat> rgbFrames;
  rgbFrames.reserve(nFrames);
  for (int i = 0; i < nFrames; i++) {
    rgbFrames.emplace(rgbFrames.begin() + i, 480, 640, CV_8UC3);
    cv::randu(rgbFrames[i], cv::Scalar::all(0), cv::Scalar::all(255));
    writer->WriteFrame(rgbFrames[i]);
  }
  delete writer;
  VisionLogReader reader(fs::path("./rgbFrames.log"));
  for (int i = 0; i < nFrames; i++) {
    std::optional<cv::Mat> curFrame = reader.TryReadFrame();
    EXPECT_TRUE(curFrame.has_value());
    EXPECT_EQ(curFrame.value().size(), rgbFrames[i].size());
    EXPECT_EQ(curFrame.value().type(), rgbFrames[i].type());
    EXPECT_EQ(cv::norm(curFrame.value(), rgbFrames[i]), 0);
  }
  reader.SkipToFrame(1);
  std::optional<cv::Mat> curFrame = reader.TryReadFrame();
  EXPECT_EQ(curFrame.value().size(), rgbFrames[1].size());
  EXPECT_EQ(curFrame.value().type(), rgbFrames[1].type());
  EXPECT_EQ(cv::norm(curFrame.value(), rgbFrames[1]), 0);
  // skip to end
  for (int i = 2; i < nFrames; i++) {
    reader.ReadFrame();
  }
  std::optional<cv::Mat> noFrame = reader.TryReadFrame();
  EXPECT_FALSE(noFrame.has_value());
  reader.SkipToFrame(0);
  std::optional<cv::Mat> frame = reader.TryReadFrame();
  EXPECT_TRUE(frame.has_value());
}

TEST(FrameLog, GrayscaleFrames) {
  int nFrames = 3;
  VisionLogWriter *writer = new VisionLogWriter(
      fs::path("./grayscaleFrames.log"), LogFrameType::GRAYSCALE, 640, 480);
  std::vector<cv::Mat> grayscaleFrames;
  grayscaleFrames.reserve(nFrames);
  for (int i = 0; i < nFrames; i++) {
    grayscaleFrames.emplace(grayscaleFrames.begin() + i, 480, 640, CV_8UC1);
    cv::randu(grayscaleFrames[i], cv::Scalar::all(0), cv::Scalar::all(255));
    writer->WriteFrame(grayscaleFrames[i]);
  }
  delete writer;
  VisionLogReader reader(fs::path("./grayscaleFrames.log"));
  for (int i = 0; i < nFrames; i++) {
    std::optional<cv::Mat> curFrame = reader.TryReadFrame();
    EXPECT_EQ(curFrame.value().size(), grayscaleFrames[i].size());
    EXPECT_EQ(curFrame.value().type(), grayscaleFrames[i].type());
    EXPECT_EQ(cv::norm(*curFrame, grayscaleFrames[i]), 0);
  }
  reader.SkipToFrame(1);
  std::optional<cv::Mat> curFrame = reader.TryReadFrame();
  EXPECT_EQ(curFrame.value().size(), grayscaleFrames[1].size());
  EXPECT_EQ(curFrame.value().type(), grayscaleFrames[1].type());
  EXPECT_EQ(cv::norm(*curFrame, grayscaleFrames[1]), 0);
  // skip to end
  for (int i = 2; i < nFrames; i++) {
    reader.ReadFrame();
  }
  std::optional<cv::Mat> noFrame = reader.TryReadFrame();
  EXPECT_FALSE(noFrame.has_value());
  reader.SkipToFrame(0);
  std::optional<cv::Mat> frame = reader.TryReadFrame();
  EXPECT_TRUE(frame.has_value());
}