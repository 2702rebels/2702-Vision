#include "object.h"
#include <gtest/gtest.h>
#include <opencv2/core/core.hpp>
#include <array>


TEST(ObjectDetection, SortPoints){
    cv::Point2f unsorted[4] = {cv::Point2f(10, 5), cv::Point2f(5, 0), cv::Point2f(10, 0), cv::Point2f(5, 5)};
    std::array<cv::Point2f, 4> sorted = imgProc::OrderPointArray(unsorted);
    std::array<cv::Point2f, 4> expected = {cv::Point2f(5, 5), cv::Point2f(10, 5), cv::Point2f(10, 0), cv::Point2f(5, 0)};

    for (size_t i = 0; i < sorted.size(); ++i) {
        EXPECT_EQ(sorted[i], expected[i]) << "Mismatch at index " << i;
    }
}