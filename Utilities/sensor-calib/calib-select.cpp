#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

namespace fs = std::filesystem;

// ─── Per-image analysis result ───────────────────────────────────────────────

struct ImageMetrics {
    std::string path;
    bool        boardFound    = false;
    double      sharpness     = 0.0;   // Laplacian variance
    double      areaFraction  = 0.0;   // board hull area / image area
    cv::Point2f boardCenter   = {0,0}; // normalised [0,1]×[0,1]
    cv::Vec3d   rvec          = {};    // from solvePnP
    double      pitchDeg      = 0.0;
    double      yawDeg        = 0.0;
    double      cornerSpread  = 0.0;   // fraction of image diagonal covered
    int         gridRow       = 0;     // 5×5 grid cell
    int         gridCol       = 0;
    int         orientBucket  = 0;     // orientation category
    cv::Size    imageSize     = {};    // original image dimensions
    std::vector<cv::Point2f> corners;
    std::vector<cv::Point3f> objPoints;
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

static const char* IMAGE_EXTS[] = {
    ".png", ".jpg", ".jpeg", ".bmp", ".tiff", ".tif", nullptr
};

static bool isImageFile(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (int i = 0; IMAGE_EXTS[i]; i++)
        if (ext == IMAGE_EXTS[i]) return true;
    return false;
}

static bool isUsableFile(const fs::directory_entry& e) {
    if (e.is_regular_file()) return true;
    return e.is_symlink() && fs::is_regular_file(e.path());
}

static std::vector<std::string> findImages(const std::vector<std::string>& paths, bool recursive) {
    std::vector<std::string> out;
    for (auto& p : paths) {
        fs::path fp(p);
        if (fs::is_directory(fp)) {
            if (recursive) {
                for (auto& e : fs::recursive_directory_iterator(fp))
                    if (isUsableFile(e) && isImageFile(e.path()))
                        out.push_back(e.path().string());
            } else {
                for (auto& e : fs::directory_iterator(fp))
                    if (isUsableFile(e) && isImageFile(e.path()))
                        out.push_back(e.path().string());
            }
        } else if (fs::exists(fp) || fs::is_symlink(fp)) {
            // Direct file argument — check it's an image
            fs::directory_entry e(fp);
            if (isUsableFile(e) && isImageFile(fp))
                out.push_back(fp.string());
            else
                fprintf(stderr, "Warning: skipping '%s' (not a supported image file)\n", p.c_str());
        } else {
            fprintf(stderr, "Warning: '%s' does not exist, skipping\n", p.c_str());
        }
    }
    std::sort(out.begin(), out.end());
    // Deduplicate (in case a file is matched by both a glob and a directory arg)
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// Laplacian variance – standard sharpness metric
static double computeSharpness(const cv::Mat& gray, const std::vector<cv::Point2f>& corners) {
    // Compute sharpness in the region around the detected board for relevance
    cv::Rect roi = cv::boundingRect(corners);
    // Expand ROI by 20% but clamp to image bounds
    int padX = roi.width  / 5;
    int padY = roi.height / 5;
    roi.x = std::max(0, roi.x - padX);
    roi.y = std::max(0, roi.y - padY);
    roi.width  = std::min(gray.cols - roi.x, roi.width  + 2 * padX);
    roi.height = std::min(gray.rows - roi.y, roi.height + 2 * padY);

    cv::Mat region = gray(roi);
    cv::Mat lap;
    cv::Laplacian(region, lap, CV_64F);
    cv::Scalar mu, sigma;
    cv::meanStdDev(lap, mu, sigma);
    return sigma[0] * sigma[0]; // variance
}

// Orientation bucket from pitch/yaw angles
static int orientationBucket(double pitchDeg, double yawDeg) {
    double ap = std::abs(pitchDeg);
    double ay = std::abs(yawDeg);
    // Near-frontal
    if (ap < 15.0 && ay < 15.0) return 0;
    // Primarily tilted up/down
    if (ap > ay && ap >= 15.0) return (pitchDeg > 0) ? 1 : 2;
    // Primarily tilted left/right
    if (ay >= 15.0) return (yawDeg > 0) ? 3 : 4;
    return 0;
}

static const char* orientLabel(int bucket) {
    switch (bucket) {
        case 0: return "frontal";
        case 1: return "tilt-up";
        case 2: return "tilt-dn";
        case 3: return "tilt-R";
        case 4: return "tilt-L";
        default: return "?";
    }
}

static std::string positionLabel(int row, int col) {
    const char* vLabels[] = { "top", "upper", "mid", "lower", "bot" };
    const char* hLabels[] = { "L", "cL", "C", "cR", "R" };
    return std::string(vLabels[row]) + "-" + hLabels[col];
}

// ─── Analyse a single image ─────────────────────────────────────────────────

static ImageMetrics analyzeImage(const std::string& path, cv::Size boardSize,
                                 float squareSize) {
    ImageMetrics m;
    m.path = path;

    cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
    if (img.empty()) return m;

    m.imageSize = img.size();

    // Detect checkerboard
    bool found = cv::findChessboardCorners(img, boardSize, m.corners,
                    cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE |
                    cv::CALIB_CB_FAST_CHECK);
    if (!found || (int)m.corners.size() != boardSize.width * boardSize.height)
        return m;

    // Sub-pixel refinement
    cv::cornerSubPix(img, m.corners, cv::Size(11, 11), cv::Size(-1, -1),
        cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.01));

    m.boardFound = true;

    // Build object points
    for (int i = 0; i < boardSize.height; i++)
        for (int j = 0; j < boardSize.width; j++)
            m.objPoints.push_back(cv::Point3f(j * squareSize, i * squareSize, 0.0f));

    // Sharpness
    m.sharpness = computeSharpness(img, m.corners);

    // Board area fraction via convex hull
    std::vector<cv::Point2f> hull;
    cv::convexHull(m.corners, hull);
    double hullArea = cv::contourArea(hull);
    double imgArea  = (double)img.cols * img.rows;
    m.areaFraction = hullArea / imgArea;

    // Board center (normalised)
    cv::Point2f sum(0, 0);
    for (auto& c : m.corners) { sum.x += c.x; sum.y += c.y; }
    m.boardCenter.x = (sum.x / m.corners.size()) / img.cols;
    m.boardCenter.y = (sum.y / m.corners.size()) / img.rows;

    // Corner spread – fraction of image diagonal covered by board diagonal
    cv::Rect br = cv::boundingRect(m.corners);
    double boardDiag = std::sqrt((double)br.width * br.width + (double)br.height * br.height);
    double imgDiag   = std::sqrt((double)img.cols * img.cols + (double)img.rows * img.rows);
    m.cornerSpread = boardDiag / imgDiag;

    // Grid cell (5×5)
    m.gridRow = std::clamp((int)(m.boardCenter.y * 5), 0, 4);
    m.gridCol = std::clamp((int)(m.boardCenter.x * 5), 0, 4);

    // Orientation via solvePnP
    cv::Mat cameraMatrix = (cv::Mat_<double>(3,3) <<
        img.cols, 0, img.cols / 2.0,
        0, img.cols, img.rows / 2.0,   // rough focal length ≈ image width
        0, 0, 1);
    cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_64F);
    cv::Mat rvec, tvec;
    cv::solvePnP(m.objPoints, m.corners, cameraMatrix, distCoeffs, rvec, tvec);

    m.rvec[0] = rvec.at<double>(0);
    m.rvec[1] = rvec.at<double>(1);
    m.rvec[2] = rvec.at<double>(2);

    // Convert rotation vector to Euler-ish angles (pitch = X rotation, yaw = Y rotation)
    cv::Mat R;
    cv::Rodrigues(rvec, R);
    m.pitchDeg = std::atan2(R.at<double>(2,1), R.at<double>(2,2)) * 180.0 / M_PI;
    m.yawDeg   = std::atan2(-R.at<double>(2,0),
                    std::sqrt(R.at<double>(2,1)*R.at<double>(2,1) +
                              R.at<double>(2,2)*R.at<double>(2,2))) * 180.0 / M_PI;

    m.orientBucket = orientationBucket(m.pitchDeg, m.yawDeg);

    return m;
}

// ─── Weighted spatial grid ──────────────────────────────────────────────────

static int cellWeight(int row, int col) {
    // Periphery-weighted 5×5 grid:
    //   3 3 3 3 3
    //   3 2 2 2 3
    //   3 2 1 2 3
    //   3 2 2 2 3
    //   3 3 3 3 3
    bool edge = (row == 0 || row == 4 || col == 0 || col == 4);
    bool inner = (row >= 1 && row <= 3 && col >= 1 && col <= 3);
    bool center = (row == 2 && col == 2);
    if (center) return 1;
    if (edge)   return 3;
    if (inner)  return 2;
    return 2;
}

// ─── Compute per-image reprojection errors after calibration ────────────────

static std::vector<double> perImageReprojError(
        const std::vector<std::vector<cv::Point3f>>& objPts,
        const std::vector<std::vector<cv::Point2f>>& imgPts,
        const cv::Mat& camMat, const cv::Mat& distC,
        const std::vector<cv::Mat>& rvecs, const std::vector<cv::Mat>& tvecs) {
    std::vector<double> errors(objPts.size());
    for (size_t i = 0; i < objPts.size(); i++) {
        std::vector<cv::Point2f> projected;
        cv::projectPoints(objPts[i], rvecs[i], tvecs[i], camMat, distC, projected);
        double err = 0;
        for (size_t j = 0; j < projected.size(); j++) {
            double dx = projected[j].x - imgPts[i][j].x;
            double dy = projected[j].y - imgPts[i][j].y;
            err += dx*dx + dy*dy;
        }
        errors[i] = std::sqrt(err / projected.size());
    }
    return errors;
}

// ─── Greedy diversity-maximising selection + calibration validation ──────────

static std::vector<size_t> selectOptimal(std::vector<ImageMetrics>& candidates,
                                         int targetCount, cv::Size imageSize) {
    if (candidates.empty()) return {};

    // Normalise sharpness and area to [0,1]
    double maxSharp = 0;
    double maxArea  = 0;
    for (auto& c : candidates) {
        maxSharp = std::max(maxSharp, c.sharpness);
        maxArea  = std::max(maxArea, c.areaFraction);
    }
    if (maxSharp < 1e-9) maxSharp = 1.0;
    if (maxArea  < 1e-9) maxArea  = 1.0;

    // Track how many selected images are in each grid cell and orient bucket
    int cellCount[5][5] = {};
    int orientCount[5]  = {};

    std::vector<bool> used(candidates.size(), false);
    std::vector<size_t> selected;

    // Composite candidate score: area(4) + spatial(3) + orientation(2) + sharpness(1)
    auto scoreCandidate = [&](size_t i) -> double {
        auto& c = candidates[i];
        int r = c.gridRow, cl = c.gridCol;
        double spatialScore = (double)cellWeight(r, cl) / (1.0 + cellCount[r][cl]);
        double orientScore  = 1.0 / (1.0 + orientCount[c.orientBucket]);
        double sharpNorm    = c.sharpness / maxSharp;
        double areaNorm     = std::sqrt(c.areaFraction / maxArea);
        return areaNorm * 4.0 + spatialScore * 3.0 + orientScore * 2.0 + sharpNorm * 1.0;
    };

    while ((int)selected.size() < targetCount) {
        double bestScore = -1;
        int    bestIdx   = -1;

        for (size_t i = 0; i < candidates.size(); i++) {
            if (used[i]) continue;
            double score = scoreCandidate(i);
            if (score > bestScore) {
                bestScore = score;
                bestIdx   = (int)i;
            }
        }

        if (bestIdx < 0) break;

        used[bestIdx] = true;
        selected.push_back(bestIdx);
        auto& c = candidates[bestIdx];
        cellCount[c.gridRow][c.gridCol]++;
        orientCount[c.orientBucket]++;
    }

    // ── Calibration validation: drop high-reprojection-error outliers ────
    // Remove images whose reprojection error exceeds a relative threshold
    // (2× median, floored at 0.5 px to avoid over-pruning).
    const int MAX_VALIDATION_ROUNDS = (int)candidates.size();
    for (int round = 0; round < MAX_VALIDATION_ROUNDS && selected.size() >= 4; round++) {
        // Build calibration inputs from selected set
        std::vector<std::vector<cv::Point3f>> objPts;
        std::vector<std::vector<cv::Point2f>> imgPts;
        for (auto idx : selected) {
            objPts.push_back(candidates[idx].objPoints);
            imgPts.push_back(candidates[idx].corners);
        }

        cv::Mat camMat, distC;
        std::vector<cv::Mat> rvecs, tvecs;
        cv::calibrateCamera(objPts, imgPts, imageSize, camMat, distC, rvecs, tvecs);

        auto errors = perImageReprojError(objPts, imgPts, camMat, distC, rvecs, tvecs);

        // Compute median error
        std::vector<double> sorted_err = errors;
        std::sort(sorted_err.begin(), sorted_err.end());
        double medianErr = sorted_err[sorted_err.size() / 2];

        // Find worst outlier exceeding relative threshold
        double threshold = std::max(2.0 * medianErr, 0.5);
        double worstErr = 0;
        int worstPos = -1;
        for (size_t i = 0; i < errors.size(); i++) {
            if (errors[i] > threshold && errors[i] > worstErr) {
                worstErr = errors[i];
                worstPos = (int)i;
            }
        }

        if (worstPos < 0) break; // No outliers, done

        // Remove outlier and try to replace with next best candidate
        size_t removedIdx = selected[worstPos];
        selected.erase(selected.begin() + worstPos);
        fprintf(stderr, "  Validation round %d: removed %s (reproj error %.2f px, median %.2f px)\n",
                round + 1, candidates[removedIdx].path.c_str(), worstErr, medianErr);

        // Update counts for removed image
        cellCount[candidates[removedIdx].gridRow][candidates[removedIdx].gridCol]--;
        orientCount[candidates[removedIdx].orientBucket]--;

        // Try to find a replacement
        double bestScore = -1;
        int bestReplace = -1;
        for (size_t i = 0; i < candidates.size(); i++) {
            if (used[i]) continue;
            double score = scoreCandidate(i);
            if (score > bestScore) {
                bestScore = score;
                bestReplace = (int)i;
            }
        }

        if (bestReplace >= 0) {
            used[bestReplace] = true;
            selected.push_back(bestReplace);
            cellCount[candidates[bestReplace].gridRow][candidates[bestReplace].gridCol]++;
            orientCount[candidates[bestReplace].orientBucket]++;
        }
    }

    return selected;
}

// ─── Output ─────────────────────────────────────────────────────────────────

static void printResults(const std::vector<ImageMetrics>& candidates,
                         const std::vector<size_t>& selected,
                         cv::Size imageSize,
                         int totalScanned, int totalDetected, int totalFiltered,
                         bool pipeMode, bool verbose) {
    if (pipeMode) {
        for (auto idx : selected)
            printf("%s\n", candidates[idx].path.c_str());
        return;
    }

    printf("Scanned %d images, %d with valid checkerboard detection\n",
           totalScanned, totalDetected);
    printf("After quality filtering: %d candidates\n", totalFiltered);

    if (selected.empty()) {
        printf("No images selected.\n");
        return;
    }

    // Run final calibration on selected set for RMS
    std::vector<std::vector<cv::Point3f>> objPts;
    std::vector<std::vector<cv::Point2f>> imgPts;
    for (auto idx : selected) {
        objPts.push_back(candidates[idx].objPoints);
        imgPts.push_back(candidates[idx].corners);
    }
    cv::Mat camMat, distC;
    std::vector<cv::Mat> rvecs, tvecs;
    double rms = cv::calibrateCamera(objPts, imgPts, imageSize, camMat, distC, rvecs, tvecs);
    auto errors = perImageReprojError(objPts, imgPts, camMat, distC, rvecs, tvecs);

    printf("Selected %d images for optimal calibration:\n\n", (int)selected.size());
    printf("  # | %-40s | Sharp | Area%%  | %-10s | %-8s | ReProj\n",
           "File", "Position", "Tilt");
    printf("----+%-42s+-------+--------+%-12s+%-10s+-------\n",
           "------------------------------------------",
           "------------", "----------");

    for (size_t i = 0; i < selected.size(); i++) {
        auto& m = candidates[selected[i]];
        printf(" %2d | %-40s | %5.0f | %5.1f%% | %-10s | %-8s | %.2f\n",
               (int)(i + 1),
               m.path.c_str(),
               m.sharpness,
               m.areaFraction * 100.0,
               positionLabel(m.gridRow, m.gridCol).c_str(),
               orientLabel(m.orientBucket),
               errors[i]);
    }

    printf("\nValidation calibration RMS: %.4f px\n\n", rms);

    // Print ready-to-run command
    printf("To calibrate, run:\n  sensor-calib");
    for (auto idx : selected)
        printf(" %s", candidates[idx].path.c_str());
    printf("\n");

    // Verbose: show all candidates
    if (verbose) {
        printf("\n--- All analysed images ---\n\n");
        printf("  # | %-40s | Board | Sharp | Area%%  | %-10s | %-8s\n",
               "File", "Position", "Tilt");
        printf("----+%-42s+-------+-------+--------+%-12s+%-10s\n",
               "------------------------------------------",
               "------------", "----------");
        for (size_t i = 0; i < candidates.size(); i++) {
            auto& m = candidates[i];
            if (!m.boardFound) {
                printf(" %3d | %-40s | NO    |       |        |            |\n",
                       (int)(i + 1), m.path.c_str());
            } else {
                printf(" %3d | %-40s | YES   | %5.0f | %5.1f%% | %-10s | %-8s\n",
                       (int)(i + 1), m.path.c_str(),
                       m.sharpness,
                       m.areaFraction * 100.0,
                       positionLabel(m.gridRow, m.gridCol).c_str(),
                       orientLabel(m.orientBucket));
            }
        }
    }
}

// ─── Usage ──────────────────────────────────────────────────────────────────

static void printUsage(const char* prog) {
    fprintf(stderr,
        "Usage: %s [options] <path> [path ...]\n\n"
        "Analyse calibration images and select the optimal subset for\n"
        "best lens characterisation.\n\n"
        "Paths can be directories (scanned for images) or individual image files.\n"
        "Shell globs are expanded before reaching this program, so patterns like\n"
        "**/*.jpg work as expected.\n\n"
        "Options:\n"
        "  -r, --recursive       Scan directories recursively\n"
        "  -n, --count <N>       Number of images to select (default: 15)\n"
        "  -b, --board <WxH>     Board size as WxH inner corners (default: 9x6)\n"
        "  -s, --square-size <F> Square size in mm (default: 30.0)\n"
        "  -p, --pipe            Machine-readable output (one filename per line)\n"
        "  -v, --verbose         Print metrics for all analysed images\n"
        "  -h, --help            Show this help\n\n"
        "Examples:\n"
        "  %s ./images/\n"
        "  %s -r -n 20 ./images/ ./more-images/\n"
        "  %s img1.jpg img2.jpg img3.jpg\n"
        "  %s images/**/*.png\n"
        "  sensor-calib $(%s -p ./images/)\n",
        prog, prog, prog, prog, prog, prog);
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    bool recursive = false;
    bool pipeMode  = false;
    bool verbose   = false;
    int  targetCount = 15;
    cv::Size boardSize(9, 6);
    float squareSize = 30.0f;
    std::vector<std::string> paths;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-r" || arg == "--recursive") {
            recursive = true;
        } else if (arg == "-p" || arg == "--pipe") {
            pipeMode = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-n" || arg == "--count") {
            if (++i >= argc) { fprintf(stderr, "Error: -n requires a value\n"); return 1; }
            targetCount = std::atoi(argv[i]);
            if (targetCount < 1) { fprintf(stderr, "Error: count must be >= 1\n"); return 1; }
        } else if (arg == "-b" || arg == "--board") {
            if (++i >= argc) { fprintf(stderr, "Error: -b requires a value (e.g. 9x6)\n"); return 1; }
            int w, h;
            if (sscanf(argv[i], "%dx%d", &w, &h) != 2 || w < 2 || h < 2) {
                fprintf(stderr, "Error: invalid board size '%s' (expected WxH, e.g. 9x6)\n", argv[i]);
                return 1;
            }
            boardSize = cv::Size(w, h);
        } else if (arg == "-s" || arg == "--square-size") {
            if (++i >= argc) { fprintf(stderr, "Error: -s requires a value\n"); return 1; }
            squareSize = std::atof(argv[i]);
            if (squareSize <= 0.0f) { fprintf(stderr, "Error: square size must be > 0\n"); return 1; }
        } else if (arg[0] == '-') {
            fprintf(stderr, "Error: unknown option '%s'\n", arg.c_str());
            printUsage(argv[0]);
            return 1;
        } else {
            paths.push_back(arg);
        }
    }

    if (paths.empty()) {
        fprintf(stderr, "Error: no files or directories specified\n\n");
        printUsage(argv[0]);
        return 1;
    }

    // ── Step 1: Find images ──────────────────────────────────────────────
    auto imageFiles = findImages(paths, recursive);
    int totalScanned = (int)imageFiles.size();

    if (totalScanned == 0) {
        fprintf(stderr, "No image files found in the specified paths\n");
        return 1;
    }
    fprintf(stderr, "Found %d image files, analysing...\n", totalScanned);

    // ── Step 2: Analyse each image ───────────────────────────────────────
    std::vector<ImageMetrics> allMetrics;
    allMetrics.reserve(totalScanned);
    cv::Size imageSize;

    for (int i = 0; i < totalScanned; i++) {
        fprintf(stderr, "  [%d/%d] %s", i + 1, totalScanned, imageFiles[i].c_str());

        ImageMetrics m = analyzeImage(imageFiles[i], boardSize, squareSize);
        if (m.boardFound) {
            fprintf(stderr, " - board found (sharp=%.0f area=%.1f%%)\n",
                    m.sharpness, m.areaFraction * 100.0);
            // Capture image size from first valid image
            if (imageSize.width == 0) {
                imageSize = m.imageSize;
            }
        } else {
            fprintf(stderr, " - no board detected\n");
        }
        allMetrics.push_back(std::move(m));
    }

    // ── Step 3: Filter candidates ────────────────────────────────────────
    int totalDetected = 0;
    std::vector<ImageMetrics> candidates;

    // Collect all with board found for sharpness threshold computation
    std::vector<double> sharpValues;
    for (auto& m : allMetrics) {
        if (m.boardFound) {
            totalDetected++;
            sharpValues.push_back(m.sharpness);
        }
    }

    if (totalDetected == 0) {
        fprintf(stderr, "\nNo checkerboard patterns detected in any image.\n"
                        "Check that --board size matches your checkerboard (currently %dx%d inner corners).\n",
                boardSize.width, boardSize.height);
        return 1;
    }

    // Adaptive sharpness threshold: bottom 20th percentile
    std::sort(sharpValues.begin(), sharpValues.end());
    double sharpThreshold = sharpValues[sharpValues.size() / 5]; // 20th percentile

    for (auto& m : allMetrics) {
        if (!m.boardFound) continue;
        if (m.sharpness < sharpThreshold) continue;
        if (m.areaFraction < 0.005 || m.areaFraction > 0.50) continue;
        candidates.push_back(std::move(m));
    }
    int totalFiltered = (int)candidates.size();

    fprintf(stderr, "Detected: %d, after quality filter: %d\n", totalDetected, totalFiltered);

    if (candidates.empty()) {
        fprintf(stderr, "No candidates passed quality filtering.\n");
        return 1;
    }

    // Clamp target to available candidates
    if (targetCount > totalFiltered)
        targetCount = totalFiltered;

    // ── Step 4: Select optimal subset ────────────────────────────────────
    fprintf(stderr, "Selecting %d optimal images...\n", targetCount);
    auto selected = selectOptimal(candidates, targetCount, imageSize);

    // ── Step 5: Output ───────────────────────────────────────────────────
    printResults(candidates, selected, imageSize,
                 totalScanned, totalDetected, totalFiltered,
                 pipeMode, verbose);

    return 0;
}
