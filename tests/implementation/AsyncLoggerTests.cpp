#include <AsyncVisionLog.h>
#include <VisionLog.h>
#include <gtest/gtest.h>
#include <memory>
#include <opencv2/opencv.hpp>

TEST(AsyncVisionLog, RGBFramesQueueSync) {
    int nFrames = 3;
    AsyncVisionLog* writer = new AsyncVisionLog(fs::path("./asyncLogRGBSync.log"),
                                                640, 480, LogFrameType::RGB, 1);
    std::vector<cv::Mat> rgbFrames;
    rgbFrames.reserve(nFrames);
    for (int i = 0; i < nFrames; i++) {
        rgbFrames.emplace(rgbFrames.begin() + i, cv::Mat(480, 640, CV_8UC3));
        cv::randu(rgbFrames[i], cv::Scalar::all(0), cv::Scalar::all(255));
        writer->AddFrameToQueueSync(rgbFrames[i]);
    }
    delete writer;
    VisionLogReader reader(fs::path("./asyncLogRGBSync.log"));
    for (int i = 0; i < nFrames; i++) {
        cv::Mat curFrame = reader.ReadFrame();
        EXPECT_EQ(curFrame.size(), rgbFrames[i].size());
        EXPECT_EQ(curFrame.type(), rgbFrames[i].type());
        EXPECT_EQ(cv::norm(curFrame, rgbFrames[i]), 0);
    }
}

TEST(AsyncVisionLog, RGBFramesQueueASync) {
    int nFrames = 3;
    AsyncVisionLog* writer = new AsyncVisionLog(
        fs::path("./asyncLogRGBASync.log"), 640, 480, LogFrameType::RGB, 2);
    std::vector<cv::Mat> rgbFrames;
    rgbFrames.reserve(nFrames);
    for (int i = 0; i < nFrames; i++) {
        rgbFrames.emplace(rgbFrames.begin() + i, cv::Mat(480, 640, CV_8UC3));
        cv::randu(rgbFrames[i], cv::Scalar::all(0), cv::Scalar::all(255));
        writer->AddFrameToQueueAsync(rgbFrames[i]);
    }
    delete writer;
    VisionLogReader reader(fs::path("./asyncLogRGBASync.log"));
    for (int i = 0; i < nFrames; i++) {
        cv::Mat curFrame = reader.ReadFrame();
        EXPECT_EQ(curFrame.size(), rgbFrames[i].size());
        EXPECT_EQ(curFrame.type(), rgbFrames[i].type());
        EXPECT_EQ(cv::norm(curFrame, rgbFrames[i]), 0);
    }
}