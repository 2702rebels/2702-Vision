#include <gtest/gtest.h>
#include <VisionLog.h>

TEST(FrameLog, GrayscaleHeader) {
	VisionLogWriter* writer = new VisionLogWriter(fs::path("./grayscaleHeader.log"), LogFrameType::GRAYSCALE, 250, 750);
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
	VisionLogWriter* writer = new VisionLogWriter(fs::path("./rgbHeader.log"), LogFrameType::RGB, 1920, 1080);
	EXPECT_EQ(writer->GetLogType(), LogFrameType::RGB);
	EXPECT_EQ(writer->GetWidth(), 1920);
	EXPECT_EQ(writer->GetHeight(), 1080);
	delete writer;
	VisionLogReader reader(fs::path("./rgbHeader.log"));
	EXPECT_EQ(reader.GetLogType(), LogFrameType::RGB);
	EXPECT_EQ(reader.GetWidth(), 1920);
	EXPECT_EQ(reader.GetHeight(), 1080);
}

TEST(FrameLog, RGBFrame) {

}

TEST(FrameLog, GrayscaleFrame) {

}