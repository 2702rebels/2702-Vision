#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fmt/format.h>

int main(int argc, char **argv) {
    // 1. Settings
    cv::Size boardSize(9, 6); // Number of internal corners (width, height)
    std::vector<std::string> imageFiles;
	for (int i=1; i <argc;i++) {
		imageFiles.emplace_back(argv[i]);
	}

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;
    cv::Size imageSize;

    // 2. Prepare 3D object points (0,0,0), (1,0,0), (2,0,0) ...
    std::vector<cv::Point3f> objp;
    for (int i = 0; i < boardSize.height; i++) {
        for (int j = 0; j < boardSize.width; j++)
        {
	        float squareSize = 30.0F;
	        objp.emplace_back(j * squareSize, i * squareSize, 0.0F);
        }
    }

    // 3. Process Images
    for (const auto& file : imageFiles) {
        cv::Mat img = cv::imread(file, cv::IMREAD_GRAYSCALE);
        if (img.empty()) continue;
        imageSize = img.size();

        cv::Mat gray = img.clone();
//        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> corners;
        // Detect chessboard corners
        bool found = cv::findChessboardCorners(gray, boardSize, corners,
                        cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);

        if (found) {
            // Refine corner locations for sub-pixel accuracy
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1));

            imagePoints.push_back(corners);
            objectPoints.push_back(objp);
        }
    }

    // 4. Run Calibration
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    std::vector<cv::Mat> rvecs;
    std::vector<cv::Mat> tvecs;
    double rms = cv::calibrateCamera(objectPoints, imagePoints, imageSize,
                                     cameraMatrix, distCoeffs, rvecs, tvecs);

	fmt::println( "1280 800" );
	for( int r = 0; r < 3; r++) {
		for( int c = 0; c < 3; c++ ) {
			fmt::print( "{:.8f} ", cameraMatrix.at<double>( r, c ) );
		}
		fmt::println( "" );
	}
	for( int d = 0; d < 5; d++) {
		fmt::print( "{:.8f} ", distCoeffs.at<double>(d) );
	}
	fmt::println( "" );
	fmt::println( "1 0 0 0" );
	fmt::println( "0 0 0" );

    // 5. Save Results
    cv::FileStorage fs("calibration.yml", cv::FileStorage::WRITE);
    fs << "camera_matrix" << cameraMatrix;
    fs << "distortion_coefficients" << distCoeffs;
    fs << "rms_error" << rms;

    std::cout << "Calibration complete. RMS error: " << rms << std::endl;
    return 0;
}