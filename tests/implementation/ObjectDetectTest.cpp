#include "ObjectFrameProcessor.h"

#include <array>
#include <gtest/gtest.h>
#include <opencv2/core/core.hpp>

TEST(ObjectDetection, SortPoints) {
    std::array unsorted = {cv::Point2f(10, 5), cv::Point2f(5, 0),
                           cv::Point2f(10, 0), cv::Point2f(5, 5)};
    std::array<cv::Point2f, 4> sorted = ObjectFrameProcessor<
        ObjLatencyReporting::ON, ObjVideoDebuggingInfo::ON, ObjUseGui::OFF,
        ObjMultipleObjectDetection::OFF, ObjSendPackets::OFF,
        ObjUseOutputFiltering::ON>::OrderPointArray(unsorted.data());
    std::array expected = {cv::Point2f(5, 5), cv::Point2f(10, 5),
                           cv::Point2f(10, 0), cv::Point2f(5, 0)};

    for (size_t i = 0; i < sorted.size(); ++i) {
        EXPECT_EQ(sorted[i], expected[i]) << "Mismatch at index " << i;
    }
}