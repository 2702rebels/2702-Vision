#pragma once
#include <opencv2/opencv.hpp>

void Bayer2Grey(cv::Mat &matGrey, cv::Mat &matBayer);
void Bayer2HalfGrey(cv::Mat &matGrey, const cv::Mat &matBayer);
