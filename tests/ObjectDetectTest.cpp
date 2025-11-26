#include "object.h"
#include <opencv2/core/core.hpp>


TEST(ObjectDetection, SortPoints){
    cv::Point2f unsorted[4] = {cv::Point2f(10, 5), cv::Point2f(5, 0), cv::Point2f(10, 0), cv::Point2f(5, 5)};
    cv::Point2f sorted = imgProc::OrderPointArray(unsorted);
    EXPECT_EQ(sorted, {cv::Point2f(5, 5), cv::Point2f(10, 5), cv::Point2f(10, 0), cv::Point2f(5, 0)});
}