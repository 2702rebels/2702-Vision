/*
 * AprilTag Frame Log Visualizer
 *
 * Reads FrameLogger text files and field JSON to visualize:
 * - Top-down field view with robot position and AprilTag locations
 * - Per-camera views showing detected tag corners
 * - Frame-by-frame stepping or continuous playback
 *
 * Usage:
 *   log-visualizer <framelog> [poses-file] [--field <field.json>]
 *
 * Controls:
 *   Space       - Play/Pause
 *   Right Arrow - Next frame
 *   Left Arrow  - Previous frame
 *   +/-         - Speed up/slow down playback
 *   Q/Esc       - Quit
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QStatusBar>
#include <QSlider>
#include <QPushButton>
#include <QStyle>
#include <QScreen>

// ---- Data structures ----

struct TagDetection {
    int         tagId;
    cv::Point2f corners[4];
};

struct CamFrame {
    int                         camId;
    std::vector<TagDetection>   detections;
};

struct FrameSet {
    unsigned                seq;
    std::vector<CamFrame>   cams;
};

struct FieldTag {
    int     id;
    double  x, y, z;
    double  qw, qx, qy, qz;

    double yawRad() const {
        double siny = 2.0 * (qw * qz + qx * qy);
        double cosy = 1.0 - 2.0 * (qy * qy + qz * qz);
        return atan2(siny, cosy);
    }
};

struct RobotPose {
    double x, y, z;
    double q0, qx, qy, qz;

    double yawRad() const {
        double siny = 2.0 * (q0 * qz + qx * qy);
        double cosy = 1.0 - 2.0 * (qy * qy + qz * qz);
        return atan2(siny, cosy);
    }
};

// ---- Minimal JSON field parser ----

static void skipWhitespace(const std::string &s, size_t &i) {
    while (i < s.size() && (s[i]==' '||s[i]=='\t'||s[i]=='\n'||s[i]=='\r')) i++;
}

static double readNumber(const std::string &s, size_t &i) {
    size_t start = i;
    if (i < s.size() && (s[i]=='-'||s[i]=='+')) i++;
    while (i < s.size() && (isdigit(s[i])||s[i]=='.'||s[i]=='e'||s[i]=='E'||s[i]=='+'||s[i]=='-')) i++;
    return atof(s.substr(start, i-start).c_str());
}

static bool loadField(const char *path, std::map<int,FieldTag> &tags, double &fieldLen, double &fieldWidth) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        fprintf(stderr, "Unable to open field file: %s\n", path);
        return false;
    }
    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    fieldLen   = 690.875;
    fieldWidth = 317.0;

    size_t fpos = json.find("\"field\"");
    if (fpos != std::string::npos) {
        size_t lpos = json.find("\"length\"", fpos);
        size_t wpos = json.find("\"width\"", fpos);
        if (lpos != std::string::npos) {
            lpos = json.find(':', lpos) + 1;
            skipWhitespace(json, lpos);
            fieldLen = readNumber(json, lpos) * 1000.0 / 25.4;
        }
        if (wpos != std::string::npos) {
            wpos = json.find(':', wpos) + 1;
            skipWhitespace(json, wpos);
            fieldWidth = readNumber(json, wpos) * 1000.0 / 25.4;
        }
    }

    size_t tpos = json.find("\"tags\"");
    if (tpos == std::string::npos) {
        fprintf(stderr, "No 'tags' array found in JSON\n");
        return false;
    }

    size_t pos = tpos;
    while ((pos = json.find("\"ID\"", pos)) != std::string::npos) {
        FieldTag ft = {};
        pos = json.find(':', pos) + 1;
        skipWhitespace(json, pos);
        ft.id = (int)readNumber(json, pos);

        size_t trl = json.find("\"translation\"", pos);
        if (trl == std::string::npos) break;

        auto readField = [&](const char *name, size_t after) -> double {
            size_t p = json.find(name, after);
            if (p == std::string::npos) return 0;
            p = json.find(':', p) + 1;
            skipWhitespace(json, p);
            return readNumber(json, p);
        };

        size_t trlBrace = json.find('{', trl);
        ft.x = readField("\"x\"", trlBrace) * 1000.0 / 25.4;
        ft.y = readField("\"y\"", trlBrace) * 1000.0 / 25.4;
        ft.z = readField("\"z\"", trlBrace) * 1000.0 / 25.4;

        size_t qp = json.find("\"quaternion\"", trl);
        if (qp == std::string::npos) break;
        size_t qBrace = json.find('{', qp);
        ft.qw = readField("\"W\"", qBrace);
        ft.qx = readField("\"X\"", qBrace);
        ft.qy = readField("\"Y\"", qBrace);
        ft.qz = readField("\"Z\"", qBrace);

        tags[ft.id] = ft;
        pos = qBrace + 1;
    }

    return !tags.empty();
}

// ---- Frame log parser ----

static bool loadFrameLog(const char *path, std::vector<FrameSet> &frameSets) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Unable to open frame log: %s\n", path);
        return false;
    }

    char buf[512];
    while (fgets(buf, sizeof(buf), fp)) {
        unsigned seq;
        if (sscanf(buf, "S:%u", &seq) != 1) continue;

        unsigned nCams;
        if (!fgets(buf, sizeof(buf), fp) || sscanf(buf, "Cams:%u", &nCams) != 1) break;

        FrameSet fs;
        fs.seq = seq;

        for (unsigned c = 0; c < nCams; c++) {
            unsigned nTags;
            if (!fgets(buf, sizeof(buf), fp) || sscanf(buf, "T:%u", &nTags) != 1) break;

            CamFrame cf;
            cf.camId = c;

            for (unsigned t = 0; t < nTags; t++) {
                if (!fgets(buf, sizeof(buf), fp)) break;
                TagDetection td;
                double c0x,c0y,c1x,c1y,c2x,c2y,c3x,c3y;
                if (sscanf(buf, "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                           &td.tagId,
                           &c0x,&c0y, &c1x,&c1y, &c2x,&c2y, &c3x,&c3y) != 9)
                    break;
                td.corners[0] = cv::Point2f((float)c0x,(float)c0y);
                td.corners[1] = cv::Point2f((float)c1x,(float)c1y);
                td.corners[2] = cv::Point2f((float)c2x,(float)c2y);
                td.corners[3] = cv::Point2f((float)c3x,(float)c3y);
                cf.detections.push_back(td);
            }
            fs.cams.push_back(cf);
        }
        frameSets.push_back(fs);
    }
    fclose(fp);
    return !frameSets.empty();
}

// ---- Pose file parser ----

static void loadPoses(const char *path, std::map<unsigned,RobotPose> &poses) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Unable to open pose file: %s\n", path);
        return;
    }

    char buf[512];
    while (fgets(buf, sizeof(buf), fp)) {
        char *end;
        long seq = strtol(buf, &end, 10);
        while (*end == ' ' || *end == '\t') end++;
        if (*end != '\n' && *end != '\r' && *end != '\0') continue;
        if (end == buf) continue;

        RobotPose lastPose = {};
        bool havePose = false;
        while (fgets(buf, sizeof(buf), fp)) {
            double q0,qx,qy,qz,x,y,z;
            if (sscanf(buf, "%lf %lf %lf %lf : %lf %lf %lf",
                       &q0,&qx,&qy,&qz, &x,&y,&z) == 7) {
                lastPose = {x, y, z, q0, qx, qy, qz};
                havePose = true;
            } else if (strstr(buf, "Lost Tracking")) {
                // skip
            } else {
                break;
            }
        }
        if (havePose) {
            poses[(unsigned)seq] = lastPose;
        }
    }
    fclose(fp);
}

// ---- AprilTag texture generation (36h11 family) ----

// Generate a 10x10 cell AprilTag image for the 36h11 family.
// Layout: 1-cell white margin, 1-cell black border, 6x6 data grid.
// Uses tag ID to generate a deterministic bit pattern.
static cv::Mat generateTagImage(int tagId, int cellSize = 10) {
    const int GRID = 10;
    int imgSize = GRID * cellSize;
    cv::Mat tag(imgSize, imgSize, CV_8UC3);

    // Start all white (margin)
    tag.setTo(cv::Scalar(255, 255, 255));

    // Fill each cell
    for (int r = 0; r < GRID; r++) {
        for (int c = 0; c < GRID; c++) {
            int x = c * cellSize, y = r * cellSize;
            cv::Rect cell(x, y, cellSize, cellSize);

            if (r == 0 || r == GRID-1 || c == 0 || c == GRID-1) {
                // White margin — already set
                continue;
            }
            if (r == 1 || r == GRID-2 || c == 1 || c == GRID-2) {
                // Black border
                cv::rectangle(tag, cell, cv::Scalar(0, 0, 0), -1);
                continue;
            }

            // Data region: rows 2-7, cols 2-7 → 6x6 = 36 bits
            int bitIdx = (r - 2) * 6 + (c - 2);
            // Generate a deterministic pattern from tag ID
            // Use a simple hash: mix tag ID with bit position
            unsigned seed = (unsigned)(tagId * 2654435761u) ^ (unsigned)(bitIdx * 340573321u);
            bool isWhite = (seed >> 16) & 1;

            cv::Scalar col = isWhite ? cv::Scalar(255,255,255) : cv::Scalar(0,0,0);
            cv::rectangle(tag, cell, col, -1);
        }
    }

    return tag;
}

// Cache of generated tag images
static std::map<int, cv::Mat> s_tagCache;

static const cv::Mat& getTagImage(int tagId) {
    auto it = s_tagCache.find(tagId);
    if (it != s_tagCache.end()) return it->second;
    s_tagCache[tagId] = generateTagImage(tagId, 10);
    return s_tagCache[tagId];
}

// Warp a tag image onto the output image at the given 4 corner positions
static void drawWarpedTag(cv::Mat &dst, int tagId, const cv::Point2f dstPts[4]) {
    const cv::Mat &tagImg = getTagImage(tagId);
    float sz = (float)tagImg.cols;

    // Source corners of the tag image (clockwise from top-left)
    cv::Point2f srcPts[4] = {
        {0, 0}, {sz, 0}, {sz, sz}, {0, sz}
    };

    // Compute perspective transform
    cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);

    // Warp into a temporary image
    cv::Mat warped = cv::Mat::zeros(dst.size(), dst.type());
    cv::warpPerspective(tagImg, warped, M, dst.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);

    // Create mask from warped alpha
    cv::Mat gray, mask;
    cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);
    // The tag has white margins, so we need a polygon mask
    std::vector<cv::Point> poly = {
        cv::Point((int)dstPts[0].x, (int)dstPts[0].y),
        cv::Point((int)dstPts[1].x, (int)dstPts[1].y),
        cv::Point((int)dstPts[2].x, (int)dstPts[2].y),
        cv::Point((int)dstPts[3].x, (int)dstPts[3].y)
    };
    mask = cv::Mat::zeros(dst.size(), CV_8UC1);
    cv::fillConvexPoly(mask, poly, cv::Scalar(255));

    warped.copyTo(dst, mask);
}

// ---- Drawing (cv::Mat, BGR) ----

static const cv::Scalar CAM_COLORS[] = {
    cv::Scalar(0, 255, 0),
    cv::Scalar(255, 100, 0),
    cv::Scalar(0, 100, 255),
    cv::Scalar(255, 0, 255),
};

static cv::Mat drawFieldView(int width, int height, double fieldLenIn, double fieldWidIn,
                              const std::map<int,FieldTag> &fieldTags,
                              const FrameSet &fs,
                              const std::map<unsigned,RobotPose> &poses,
                              const std::vector<FrameSet> &allFrames,
                              int currentIdx)
{
    cv::Mat img = cv::Mat::zeros(height, width, CV_8UC3);

    int margin = 30;
    int fw = width - 2*margin;
    int fh = height - 2*margin;
    double scale = std::min(fw / fieldLenIn, fh / fieldWidIn);
    int fieldW = (int)(fieldLenIn * scale);
    int fieldH = (int)(fieldWidIn * scale);
    int ox = margin + (fw - fieldW) / 2;
    int oy = margin + (fh - fieldH) / 2;

    auto fieldToPx = [&](double fx, double fy) -> cv::Point {
        return cv::Point((int)(ox + fx * scale), (int)(oy + (fieldWidIn - fy) * scale));
    };

    // Field border & center line
    cv::rectangle(img, cv::Point(ox, oy), cv::Point(ox+fieldW, oy+fieldH), cv::Scalar(60,60,60), 2);
    int cx = fieldToPx(fieldLenIn/2.0, 0).x;
    cv::line(img, cv::Point(cx, oy), cv::Point(cx, oy+fieldH), cv::Scalar(40,40,40), 1);

    // --- 2026 Rebuilt field elements (from field image) ---

    // Bumps — narrow vertical strips same width as hubs, running through each alliance's side
    // Red bump (~x=456-504, matching red hub width)
    {
        cv::Point p1 = fieldToPx(456, 60);
        cv::Point p2 = fieldToPx(504, 258);
        cv::rectangle(img, p1, p2, cv::Scalar(15, 15, 50), -1);
        cv::rectangle(img, p1, p2, cv::Scalar(30, 30, 100), 1);
    }
    // Blue bump (~x=158-206, matching blue hub width)
    {
        cv::Point p1 = fieldToPx(158, 60);
        cv::Point p2 = fieldToPx(206, 258);
        cv::rectangle(img, p1, p2, cv::Scalar(50, 15, 15), -1);
        cv::rectangle(img, p1, p2, cv::Scalar(100, 30, 30), 1);
    }

    // Hub outlines (octagonal, sitting within the bumps)
    auto drawHub = [&](double hx, double hy, cv::Scalar color, const char *label) {
        double r = 24;
        std::vector<cv::Point> pts;
        for (int i = 0; i < 8; i++) {
            double a = i * M_PI / 4 + M_PI / 8;
            pts.push_back(fieldToPx(hx + r*cos(a), hy + r*sin(a)));
        }
        cv::polylines(img, pts, true, color, 2, cv::LINE_AA);
        cv::Point p = fieldToPx(hx, hy);
        cv::putText(img, label, cv::Point(p.x-12, p.y+4), cv::FONT_HERSHEY_SIMPLEX, 0.35, color, 1);
    };
    drawHub(480, 159, cv::Scalar(50, 50, 160), "HUB");    // Red hub
    drawHub(182, 159, cv::Scalar(160, 50, 50), "HUB");    // Blue hub

    // Towers — small structures at top and bottom edges
    auto drawTower = [&](double tx, double ty, cv::Scalar color) {
        cv::Point p = fieldToPx(tx, ty);
        cv::rectangle(img, cv::Point(p.x-10, p.y-10), cv::Point(p.x+10, p.y+10), color, 1);
        cv::line(img, cv::Point(p.x-6, p.y-6), cv::Point(p.x+6, p.y+6), color, 1);
        cv::line(img, cv::Point(p.x+6, p.y-6), cv::Point(p.x-6, p.y+6), color, 1);
    };
    drawTower(469, 25, cv::Scalar(50, 50, 140));    // Red tower top (tags 6,7)
    drawTower(469, 292, cv::Scalar(50, 50, 140));    // Red tower bottom (tags 1,12)
    drawTower(182, 25, cv::Scalar(140, 50, 50));     // Blue tower top (tags 17,28)
    drawTower(182, 292, cv::Scalar(140, 50, 50));    // Blue tower bottom (tags 22,23)

    // Depots — yellow markers along top and bottom walls
    auto drawDepot = [&](double dx, double dy) {
        cv::Point p = fieldToPx(dx, dy);
        cv::rectangle(img, cv::Point(p.x-5, p.y-3), cv::Point(p.x+5, p.y+3), cv::Scalar(0, 160, 180), -1);
    };
    // Top wall depots
    for (double x : {100.0, 140.0, 220.0, 260.0, 400.0, 440.0, 520.0, 560.0})
        drawDepot(x, 8);
    // Bottom wall depots
    for (double x : {100.0, 140.0, 220.0, 260.0, 400.0, 440.0, 520.0, 560.0})
        drawDepot(x, 310);

    // Trenches — along the top wall (scoring table side)
    // Red trench (right side of top wall)
    {
        cv::Point p1 = fieldToPx(555, 0);
        cv::Point p2 = fieldToPx(640, 40);
        cv::rectangle(img, p1, p2, cv::Scalar(50, 50, 140), 1);
        cv::putText(img, "TRENCH", cv::Point((p1.x+p2.x)/2-25, (p1.y+p2.y)/2+4),
                    cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(50,50,140), 1);
    }
    // Blue trench (left side of top wall)
    {
        cv::Point p1 = fieldToPx(15, 0);
        cv::Point p2 = fieldToPx(100, 40);
        cv::rectangle(img, p1, p2, cv::Scalar(140, 50, 50), 1);
        cv::putText(img, "TRENCH", cv::Point((p1.x+p2.x)/2-25, (p1.y+p2.y)/2+4),
                    cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(140,50,50), 1);
    }

    // Outpost — on the left wall (audience side)
    {
        cv::Point p1 = fieldToPx(0, 200);
        cv::Point p2 = fieldToPx(25, 240);
        cv::rectangle(img, p1, p2, cv::Scalar(100, 100, 100), 1);
        cv::putText(img, "OUT", cv::Point((p1.x+p2.x)/2-10, (p1.y+p2.y)/2+4),
                    cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(100,100,100), 1);
    }

    // Alliance wall labels
    cv::putText(img, "BLUE", cv::Point(ox+5, oy+fieldH-5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180,80,40), 1);
    cv::putText(img, "RED", cv::Point(ox+fieldW-45, oy+fieldH-5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(40,40,180), 1);
    // Scoring table label at bottom
    cv::putText(img, "SCORING TABLE", cv::Point(ox + fieldW/2 - 55, oy + fieldH + 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(80,80,80), 1);

    // All field tags
    for (auto &[id, ft] : fieldTags) {
        cv::Point p = fieldToPx(ft.x, ft.y);
        double yaw = ft.yawRad();
        int tagLen = std::max((int)(3.25 * scale), 4);
        int dx = (int)(tagLen * cos(yaw));
        int dy = (int)(-tagLen * sin(yaw));
        cv::line(img, cv::Point(p.x-dy, p.y-dx), cv::Point(p.x+dy, p.y+dx), cv::Scalar(100,100,100), 2);
        cv::arrowedLine(img, p, cv::Point(p.x+dx, p.y-dy), cv::Scalar(80,80,80), 1, cv::LINE_8, 0, 0.4);
        cv::putText(img, std::to_string(id), cv::Point(p.x+8, p.y-8),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(120,120,120), 1);
    }

    // Highlight visible tags
    for (auto &cam : fs.cams)
        for (auto &det : cam.detections) {
            auto it = fieldTags.find(det.tagId);
            if (it == fieldTags.end()) continue;
            cv::Point p = fieldToPx(it->second.x, it->second.y);
            cv::circle(img, p, 6, cv::Scalar(0,255,255), 2);
            cv::putText(img, std::to_string(det.tagId), cv::Point(p.x+10, p.y-4),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,255,255), 2);
        }

    // Robot trail
    int trailLen = 100;
    int start = std::max(0, currentIdx - trailLen);
    for (int i = start; i <= currentIdx && i < (int)allFrames.size(); i++) {
        auto pit = poses.find(allFrames[i].seq);
        if (pit == poses.end()) continue;
        cv::Point p = fieldToPx(pit->second.x, pit->second.y);
        double alpha = (double)(i - start) / std::max(trailLen, 1);
        cv::circle(img, p, 2, cv::Scalar(50+(int)(200*alpha), 50+(int)(200*alpha), 0), -1);
    }

    // Current robot pose
    auto curIt = poses.find(fs.seq);
    if (curIt != poses.end()) {
        const RobotPose &rp = curIt->second;
        cv::Point p = fieldToPx(rp.x, rp.y);
        cv::circle(img, p, 8, cv::Scalar(0,255,0), 2);
        double yaw = rp.yawRad();
        cv::Point tip(p.x + (int)(20*cos(yaw)), p.y - (int)(20*sin(yaw)));
        cv::arrowedLine(img, p, tip, cv::Scalar(0,255,0), 2, cv::LINE_8, 0, 0.3);
    }

    // Info overlay
    int totalTags = 0;
    for (auto &cam : fs.cams) totalTags += (int)cam.detections.size();
    char info[128];
    snprintf(info, sizeof(info), "Seq: %u  Tags: %d", fs.seq, totalTags);
    cv::putText(img, info, cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(200,200,200), 2);

    if (curIt != poses.end()) {
        snprintf(info, sizeof(info), "X:%.1f  Y:%.1f  Yaw:%.1f",
                 curIt->second.x, curIt->second.y, curIt->second.yawRad() * 180.0 / M_PI);
        cv::putText(img, info, cv::Point(10, height-12), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(200,200,200), 2);
    } else if (totalTags > 0) {
        cv::putText(img, "No pose (bEstimate rejected)", cv::Point(10, height-12),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,0,200), 2);
    }

    return img;
}

static cv::Mat drawCameraViews(const FrameSet &fs, int nCams, int camW = 480, int camH = 360) {
    int cols = std::min(nCams, 2);
    int rows = (nCams + cols - 1) / cols;
    cv::Mat img = cv::Mat::zeros(rows * camH, cols * camW, CV_8UC3);

    float srcW = 1280.0f, srcH = 800.0f;
    float sx = camW / srcW, sy = camH / srcH;

    for (int c = 0; c < nCams; c++) {
        int col = c % cols, row = c / cols;
        int xOff = col * camW, yOff = row * camH;
        cv::Scalar color = CAM_COLORS[c % 4];

        cv::rectangle(img, cv::Point(xOff+1, yOff+1), cv::Point(xOff+camW-2, yOff+camH-2), cv::Scalar(60,60,60), 1);

        char label[32];
        snprintf(label, sizeof(label), "Cam %d", c);
        cv::putText(img, label, cv::Point(xOff+8, yOff+25), cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2);

        const CamFrame *cf = nullptr;
        for (auto &cam : fs.cams)
            if (cam.camId == c) { cf = &cam; break; }

        if (!cf || cf->detections.empty()) {
            cv::putText(img, "No tags", cv::Point(xOff+camW/2-50, yOff+camH/2),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(80,80,80), 2);
            continue;
        }

        snprintf(label, sizeof(label), "%d tag%s", (int)cf->detections.size(), cf->detections.size()!=1?"s":"");
        cv::putText(img, label, cv::Point(xOff+camW-110, yOff+25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(180,180,180), 2);

        for (auto &det : cf->detections) {
            cv::Point pts[4];
            cv::Point2f ptsf[4];
            for (int i = 0; i < 4; i++) {
                float px = xOff + det.corners[i].x * sx;
                float py = yOff + det.corners[i].y * sy;
                pts[i] = cv::Point((int)px, (int)py);
                ptsf[i] = cv::Point2f(px, py);
            }

            // Draw perspective-warped AprilTag texture
            drawWarpedTag(img, det.tagId, ptsf);

            // Outline on top of the texture
            for (int i = 0; i < 4; i++)
                cv::line(img, pts[i], pts[(i+1)%4], color, 1);

            // Corner dot — first corner red to show orientation
            cv::circle(img, pts[0], 3, cv::Scalar(0,0,255), -1);

            // Tag ID label above the tag
            cv::Point ctr((pts[0].x+pts[1].x+pts[2].x+pts[3].x)/4, (pts[0].y+pts[1].y+pts[2].y+pts[3].y)/4);
            int topY = std::min({pts[0].y, pts[1].y, pts[2].y, pts[3].y});
            cv::putText(img, std::to_string(det.tagId), cv::Point(ctr.x-8, topY-6), cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2);

            // Pixel area below the tag
            std::vector<cv::Point> contour(pts, pts+4);
            double area = cv::contourArea(contour);
            int botY = std::max({pts[0].y, pts[1].y, pts[2].y, pts[3].y});
            snprintf(label, sizeof(label), "%dpx", (int)area);
            cv::putText(img, label, cv::Point(ctr.x-20, botY+18), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180,180,180), 1);
        }
    }
    return img;
}

// Convert BGR cv::Mat to QImage (RGB)
static QImage matToQImage(const cv::Mat &mat) {
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, (int)rgb.step, QImage::Format_RGB888).copy();
}

// ---- Default field finder ----

static std::string findDefaultFieldJson(const char *argv0) {
    std::string exe(argv0);
    if (!exe.empty() && exe[0] != '/' && exe[0] != '\\') {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)))
            exe = std::string(cwd) + "/" + exe;
    }

    std::string dir = exe;
    for (int ups = 0; ups < 6; ups++) {
        size_t slash = dir.find_last_of("/\\");
        if (slash == std::string::npos) break;
        dir = dir.substr(0, slash);
        std::string candidate = dir + "/2026/2026AprilTag/2026-rebuilt-welded.json";
        FILE *fp = fopen(candidate.c_str(), "r");
        if (fp) { fclose(fp); return candidate; }
    }

    const char *cwdCandidates[] = {
        "2026/2026AprilTag/2026-rebuilt-welded.json",
        "../2026/2026AprilTag/2026-rebuilt-welded.json",
        "../../2026/2026AprilTag/2026-rebuilt-welded.json",
    };
    for (auto &c : cwdCandidates) {
        FILE *fp = fopen(c, "r");
        if (fp) { fclose(fp); return c; }
    }
    return "";
}

// ---- Qt Main Window ----

class VisualizerWindow : public QMainWindow {
    Q_OBJECT

public:
    VisualizerWindow(const std::map<int,FieldTag> &fieldTags,
                     double fieldLen, double fieldWidth,
                     const std::vector<FrameSet> &frameSets,
                     const std::map<unsigned,RobotPose> &poses,
                     const std::string &fieldName,
                     int nCams)
        : m_fieldTags(fieldTags), m_fieldLen(fieldLen), m_fieldWidth(fieldWidth),
          m_frameSets(frameSets), m_poses(poses), m_fieldName(fieldName),
          m_nCams(nCams), m_current(0), m_playing(false), m_delayMs(100)
    {
        setWindowTitle("AprilTag Log Visualizer");

        auto *central = new QWidget(this);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(2);

        // Field view
        m_fieldLabel = new QLabel(this);
        m_fieldLabel->setAlignment(Qt::AlignCenter);
        m_fieldLabel->setMinimumHeight(400);
        mainLayout->addWidget(m_fieldLabel, 3);

        // Camera views
        m_camLabel = new QLabel(this);
        m_camLabel->setAlignment(Qt::AlignCenter);
        m_camLabel->setMinimumHeight(300);
        mainLayout->addWidget(m_camLabel, 2);

        // Controls bar
        auto *controlBar = new QHBoxLayout();
        controlBar->setSpacing(8);

        m_playBtn = new QPushButton("Play", this);
        m_playBtn->setFixedWidth(70);
        connect(m_playBtn, &QPushButton::clicked, this, &VisualizerWindow::togglePlay);
        controlBar->addWidget(m_playBtn);

        auto *prevBtn = new QPushButton("<", this);
        prevBtn->setFixedWidth(40);
        connect(prevBtn, &QPushButton::clicked, this, &VisualizerWindow::prevFrame);
        controlBar->addWidget(prevBtn);

        m_slider = new QSlider(Qt::Horizontal, this);
        m_slider->setRange(0, (int)m_frameSets.size() - 1);
        m_slider->setValue(0);
        connect(m_slider, &QSlider::valueChanged, this, &VisualizerWindow::onSlider);
        controlBar->addWidget(m_slider);

        auto *nextBtn = new QPushButton(">", this);
        nextBtn->setFixedWidth(40);
        connect(nextBtn, &QPushButton::clicked, this, &VisualizerWindow::nextFrame);
        controlBar->addWidget(nextBtn);

        mainLayout->addLayout(controlBar);

        setCentralWidget(central);

        // Status bar
        m_statusLabel = new QLabel(this);
        statusBar()->addPermanentWidget(m_statusLabel, 1);

        // Playback timer
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &VisualizerWindow::onTimer);

        updateDisplay();
    }

protected:
    void keyPressEvent(QKeyEvent *e) override {
        switch (e->key()) {
        case Qt::Key_Space:  togglePlay(); break;
        case Qt::Key_Right:
        case Qt::Key_D:      nextFrame(); break;
        case Qt::Key_Left:
        case Qt::Key_A:      prevFrame(); break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:  m_delayMs = std::max(10, m_delayMs - 20); updateStatus(); break;
        case Qt::Key_Minus:  m_delayMs = std::min(2000, m_delayMs + 20); updateStatus(); break;
        case Qt::Key_Q:
        case Qt::Key_Escape: close(); break;
        default:             QMainWindow::keyPressEvent(e);
        }
    }

    void resizeEvent(QResizeEvent *e) override {
        QMainWindow::resizeEvent(e);
        updateDisplay();
    }

private slots:
    void togglePlay() {
        m_playing = !m_playing;
        m_playBtn->setText(m_playing ? "Pause" : "Play");
        if (m_playing)
            m_timer->start(m_delayMs);
        else
            m_timer->stop();
        updateStatus();
    }

    void nextFrame() {
        if (m_current < (int)m_frameSets.size() - 1) {
            m_current++;
            m_slider->setValue(m_current);
            updateDisplay();
        }
    }

    void prevFrame() {
        if (m_current > 0) {
            m_current--;
            m_slider->setValue(m_current);
            updateDisplay();
        }
    }

    void onSlider(int val) {
        if (val != m_current) {
            m_current = val;
            updateDisplay();
        }
    }

    void onTimer() {
        if (m_current < (int)m_frameSets.size() - 1) {
            m_current++;
            m_slider->setValue(m_current);
            m_timer->setInterval(m_delayMs);
            updateDisplay();
        } else {
            m_playing = false;
            m_playBtn->setText("Play");
            m_timer->stop();
            updateStatus();
        }
    }

private:
    void updateDisplay() {
        const FrameSet &fs = m_frameSets[m_current];

        // Draw field view sized to the label
        int fW = std::max(m_fieldLabel->width(), 400);
        int fH = std::max(m_fieldLabel->height(), 200);
        cv::Mat fieldImg = drawFieldView(fW, fH, m_fieldLen, m_fieldWidth,
                                          m_fieldTags, fs, m_poses, m_frameSets, m_current);
        QImage fqi = matToQImage(fieldImg);
        m_fieldLabel->setPixmap(QPixmap::fromImage(fqi));

        // Draw camera views sized to the label
        int cW = std::max(m_camLabel->width(), 400);
        int cH = std::max(m_camLabel->height(), 200);
        int camTileW = cW / 2;
        int camTileH = cH / ((m_nCams + 1) / 2);
        cv::Mat camImg = drawCameraViews(fs, m_nCams, camTileW, camTileH);
        // Resize to fit label
        if (camImg.cols > cW || camImg.rows > cH) {
            double s = std::min((double)cW / camImg.cols, (double)cH / camImg.rows);
            cv::resize(camImg, camImg, cv::Size((int)(camImg.cols*s), (int)(camImg.rows*s)));
        }
        QImage cqi = matToQImage(camImg);
        m_camLabel->setPixmap(QPixmap::fromImage(cqi));

        updateStatus();
    }

    void updateStatus() {
        const FrameSet &fs = m_frameSets[m_current];
        int totalTags = 0;
        for (auto &cam : fs.cams) totalTags += (int)cam.detections.size();

        QString status = QString("Frame %1/%2  |  Seq %3  |  Tags: %4  |  %5  |  Delay: %6ms  |  Field: %7")
            .arg(m_current + 1)
            .arg(m_frameSets.size())
            .arg(fs.seq)
            .arg(totalTags)
            .arg(m_playing ? "PLAYING" : "PAUSED")
            .arg(m_delayMs)
            .arg(QString::fromStdString(m_fieldName));

        auto it = m_poses.find(fs.seq);
        if (it != m_poses.end()) {
            status += QString("  |  X:%1  Y:%2  Yaw:%3")
                .arg(it->second.x, 0, 'f', 1)
                .arg(it->second.y, 0, 'f', 1)
                .arg(it->second.yawRad() * 180.0 / M_PI, 0, 'f', 1);
        }

        m_statusLabel->setText(status);
    }

    const std::map<int,FieldTag>       &m_fieldTags;
    double                              m_fieldLen, m_fieldWidth;
    const std::vector<FrameSet>        &m_frameSets;
    const std::map<unsigned,RobotPose> &m_poses;
    std::string                         m_fieldName;
    int                                 m_nCams;
    int                                 m_current;
    bool                                m_playing;
    int                                 m_delayMs;

    QLabel      *m_fieldLabel;
    QLabel      *m_camLabel;
    QLabel      *m_statusLabel;
    QPushButton *m_playBtn;
    QSlider     *m_slider;
    QTimer      *m_timer;
};

// ---- Main ----

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // Dark theme
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(40, 40, 40));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::ToolTipText, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Text, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Button, QColor(50, 50, 50));
    darkPalette.setColor(QPalette::ButtonText, QColor(200, 200, 200));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    app.setStyleSheet("QStatusBar { border-top: 1px solid #444; } "
                      "QPushButton { padding: 4px 8px; } "
                      "QSlider::groove:horizontal { height: 6px; background: #555; border-radius: 3px; } "
                      "QSlider::handle:horizontal { width: 14px; margin: -4px 0; background: #2a82da; border-radius: 7px; }");

    setbuf(stdout, NULL);

    // Parse args
    const char *fieldPath = nullptr;
    const char *logPath   = nullptr;
    const char *posePath  = nullptr;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--field") == 0 || strcmp(argv[i], "-f") == 0) && i+1 < argc) {
            fieldPath = argv[++i];
        } else if (!logPath) {
            logPath = argv[i];
        } else if (!posePath) {
            posePath = argv[i];
        } else {
            fprintf(stderr, "Usage: %s <framelog> [poses-file] [--field <field.json>]\n", argv[0]);
            return 1;
        }
    }

    if (!logPath) {
        fprintf(stderr, "Usage: %s <framelog> [poses-file] [--field <field.json>]\n", argv[0]);
        fprintf(stderr, "\nIf --field is omitted, defaults to 2026-rebuilt-welded.json\n");
        return 1;
    }

    std::string defaultField;
    if (!fieldPath) {
        defaultField = findDefaultFieldJson(argv[0]);
        if (defaultField.empty()) {
            fprintf(stderr, "Could not find default field JSON (2026-rebuilt-welded.json).\n");
            fprintf(stderr, "Specify with --field <path>\n");
            return 1;
        }
        fieldPath = defaultField.c_str();
        printf("Using default field: %s\n", fieldPath);
    }

    // Load data
    std::map<int,FieldTag> fieldTags;
    double fieldLen, fieldWidth;
    printf("Loading field from %s...\n", fieldPath);
    if (!loadField(fieldPath, fieldTags, fieldLen, fieldWidth)) return 1;
    printf("  %zu tags, field %.1f x %.1f in\n", fieldTags.size(), fieldLen, fieldWidth);

    std::vector<FrameSet> frameSets;
    printf("Loading frame log from %s...\n", logPath);
    if (!loadFrameLog(logPath, frameSets)) return 1;
    printf("  %zu frame sets\n", frameSets.size());

    std::map<unsigned,RobotPose> poses;
    if (posePath) {
        printf("Loading poses from %s...\n", posePath);
        loadPoses(posePath, poses);
        printf("  %zu poses\n", poses.size());
    }

    int nCams = 4;
    for (auto &fs : frameSets)
        nCams = std::max(nCams, (int)fs.cams.size());

    std::string fieldName(fieldPath);
    size_t lastSlash = fieldName.find_last_of("/\\");
    if (lastSlash != std::string::npos) fieldName = fieldName.substr(lastSlash + 1);

    VisualizerWindow win(fieldTags, fieldLen, fieldWidth, frameSets, poses, fieldName, nCams);

    // Size to 80% of screen
    QScreen *screen = app.primaryScreen();
    QRect screenGeom = screen->availableGeometry();
    int winW = screenGeom.width() * 80 / 100;
    int winH = screenGeom.height() * 80 / 100;
    win.resize(winW, winH);
    win.move((screenGeom.width() - winW) / 2, (screenGeom.height() - winH) / 2);
    win.show();

    return app.exec();
}

#include "log_visualizer.moc"
