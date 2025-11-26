/*! \brief Program to get the calibration data for the camera. 
Loads all png images from /data/calibration/ to create these calibration values.
* Depends on:
* - opencv
*/

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

int main(int argc, char** argv) {
    // Define the checkerboard pattern size
    Size patternSize(9, 6);  // 9x6 checkerboard
    char const *inputDir = 0;
    if(argc == 1 || argc > 2){
        std::cerr << "Usage: ./calibrate inputDir, where inputDir is a path to a directory with .jpg imgs to calibrate\n";
        std::cerr << "Example: ./calibrate /media/sf_2702_Vision/2025/Object/good/\n";
        return 1;
    }
    else{
        inputDir = argv[1];
    }
    // Create vectors to store the object points and image points
    vector<vector<Point3f>> objectPoints;
    vector<vector<Point2f>> imagePoints;

    // Define the world coordinates for 3D points
    vector<Point3f> objp;
    for (int i = 0; i < patternSize.height; ++i) {
        for (int j = 0; j < patternSize.width; ++j) {
            objp.push_back(Point3f(j, i, 0));
        }
    }

    // Load the checkerboard images
    vector<String> filenames;
    glob(std::string(inputDir) + "*.jpg", filenames);

    // Detect checkerboard corners in each image
    Mat gray;
    for (const auto& filename : filenames) {
        Mat image = imread(filename);
        cvtColor(image, gray, COLOR_BGR2GRAY);

        vector<Point2f> corners;
        bool found = findChessboardCorners(gray, patternSize, corners,
                                          CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE);

        if (found) {
            // Refine corner locations
            cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1),
                         TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 30, 0.0001));

            // Add object points and image points
            objectPoints.push_back(objp);
            imagePoints.push_back(corners);

            // Draw and display the corners (optional)
            drawChessboardCorners(image, patternSize, corners, found);
            imshow("Corners", image);
            waitKey(500);
        }
    }

    destroyAllWindows();

    // Calibrate the camera
    Mat cameraMatrix, distCoeffs, rvecs, tvecs;
    calibrateCamera(objectPoints, imagePoints, Size(gray.cols, gray.rows), cameraMatrix, distCoeffs, rvecs, tvecs);

    // Print the camera parameters
    cout << "Camera Matrix:\n" << cameraMatrix << endl;
    cout << "Distortion Coefficients:\n" << distCoeffs << endl;

    return 0;
}
