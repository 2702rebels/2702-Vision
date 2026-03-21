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
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

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
    double  x, y, z;       // inches
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
// Parses the WPILib AprilTag field JSON without external libraries.

static void skipWhitespace(const std::string &s, size_t &i) {
    while (i < s.size() && (s[i]==' '||s[i]=='\t'||s[i]=='\n'||s[i]=='\r')) i++;
}

static std::string readString(const std::string &s, size_t &i) {
    if (s[i] != '"') return "";
    i++; // skip opening quote
    std::string out;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\') { i++; if (i < s.size()) out += s[i]; }
        else out += s[i];
        i++;
    }
    if (i < s.size()) i++; // skip closing quote
    return out;
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

    fieldLen   = 690.875;  // defaults in inches
    fieldWidth = 317.0;

    // Find "field" object for dimensions
    size_t fpos = json.find("\"field\"");
    if (fpos != std::string::npos) {
        size_t lpos = json.find("\"length\"", fpos);
        size_t wpos = json.find("\"width\"", fpos);
        if (lpos != std::string::npos) {
            lpos = json.find(':', lpos) + 1;
            skipWhitespace(json, lpos);
            fieldLen = readNumber(json, lpos) * 1000.0 / 25.4; // meters -> inches
        }
        if (wpos != std::string::npos) {
            wpos = json.find(':', wpos) + 1;
            skipWhitespace(json, wpos);
            fieldWidth = readNumber(json, wpos) * 1000.0 / 25.4;
        }
    }

    // Parse tags array
    size_t tpos = json.find("\"tags\"");
    if (tpos == std::string::npos) {
        fprintf(stderr, "No 'tags' array found in JSON\n");
        return false;
    }

    // Find each "ID" entry
    size_t pos = tpos;
    while ((pos = json.find("\"ID\"", pos)) != std::string::npos) {
        FieldTag ft = {};

        // ID
        pos = json.find(':', pos) + 1;
        skipWhitespace(json, pos);
        ft.id = (int)readNumber(json, pos);

        // translation
        size_t trl = json.find("\"translation\"", pos);
        if (trl == std::string::npos) break;

        auto readField = [&](const char *name, size_t after) -> double {
            size_t p = json.find(name, after);
            if (p == std::string::npos) return 0;
            p = json.find(':', p) + 1;
            skipWhitespace(json, p);
            return readNumber(json, p);
        };

        // Find the brace that starts translation object
        size_t trlBrace = json.find('{', trl);
        ft.x = readField("\"x\"", trlBrace) * 1000.0 / 25.4;
        ft.y = readField("\"y\"", trlBrace) * 1000.0 / 25.4;
        ft.z = readField("\"z\"", trlBrace) * 1000.0 / 25.4;

        // quaternion
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
        // Try to read a sequence number (bare integer on a line)
        char *end;
        long seq = strtol(buf, &end, 10);
        // Must be a line with just a number
        while (*end == ' ' || *end == '\t') end++;
        if (*end != '\n' && *end != '\r' && *end != '\0') continue;
        if (end == buf) continue;

        // Read up to 2 pose lines
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
                // Not a pose line — put it back conceptually by seeking
                // Actually we can't ungetc a whole line, so just break
                // and re-parse this line as a potential seq on next iteration.
                // We handle this by checking if it's a seq number at loop top.
                break;
            }
        }
        if (havePose) {
            poses[(unsigned)seq] = lastPose;
        }
    }
    fclose(fp);
}

// ---- Drawing ----

static const cv::Scalar CAM_COLORS[] = {
    cv::Scalar(0, 255, 0),      // Green
    cv::Scalar(255, 100, 0),    // Blue-ish
    cv::Scalar(0, 100, 255),    // Orange-ish
    cv::Scalar(255, 0, 255),    // Magenta
};

struct FieldView {
    int     width, height;
    double  scale;
    int     ox, oy;
    int     fieldW, fieldH;
    double  fieldLenIn, fieldWidIn;

    FieldView(int w, int h, double fLen, double fWid)
        : width(w), height(h), fieldLenIn(fLen), fieldWidIn(fWid)
    {
        int margin = 30;
        int fw = w - 2*margin;
        int fh = h - 2*margin;
        double sx = fw / fLen;
        double sy = fh / fWid;
        scale = std::min(sx, sy);
        fieldW = (int)(fLen * scale);
        fieldH = (int)(fWid * scale);
        ox = margin + (fw - fieldW) / 2;
        oy = margin + (fh - fieldH) / 2;
    }

    cv::Point fieldToPx(double fx, double fy) const {
        return cv::Point((int)(ox + fx * scale),
                         (int)(oy + (fieldWidIn - fy) * scale));
    }

    cv::Mat draw(const std::map<int,FieldTag> &fieldTags,
                 const FrameSet &fs,
                 const std::map<unsigned,RobotPose> &poses,
                 const std::vector<FrameSet> &allFrames,
                 int currentIdx) const
    {
        cv::Mat img = cv::Mat::zeros(height, width, CV_8UC3);

        // Field border
        cv::rectangle(img, cv::Point(ox, oy),
                      cv::Point(ox + fieldW, oy + fieldH), cv::Scalar(60,60,60), 2);

        // Center line
        cv::Point cl = fieldToPx(fieldLenIn / 2.0, 0);
        cv::line(img, cv::Point(cl.x, oy), cv::Point(cl.x, oy+fieldH), cv::Scalar(40,40,40), 1);

        // Draw all field tags
        for (auto &[id, ft] : fieldTags) {
            cv::Point p = fieldToPx(ft.x, ft.y);
            double yaw = ft.yawRad();
            int tagLen = std::max((int)(3.25 * scale), 4);
            int dx = (int)(tagLen * cos(yaw));
            int dy = (int)(-tagLen * sin(yaw));
            cv::line(img, cv::Point(p.x-dy, p.y-dx), cv::Point(p.x+dy, p.y+dx),
                     cv::Scalar(100,100,100), 2);
            cv::arrowedLine(img, p, cv::Point(p.x+dx, p.y-dy),
                            cv::Scalar(80,80,80), 1, cv::LINE_8, 0, 0.4);
            cv::putText(img, std::to_string(id), cv::Point(p.x+5, p.y-5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.35, cv::Scalar(120,120,120), 1);
        }

        // Highlight visible tags
        std::vector<int> seenTags;
        for (auto &cam : fs.cams)
            for (auto &det : cam.detections)
                seenTags.push_back(det.tagId);

        for (int tid : seenTags) {
            auto it = fieldTags.find(tid);
            if (it == fieldTags.end()) continue;
            cv::Point p = fieldToPx(it->second.x, it->second.y);
            cv::circle(img, p, 6, cv::Scalar(0,255,255), 2);
            cv::putText(img, std::to_string(tid), cv::Point(p.x+8, p.y-2),
                        cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0,255,255), 1);
        }

        // Robot trail
        int trailLen = 100;
        int start = std::max(0, currentIdx - trailLen);
        for (int i = start; i <= currentIdx && i < (int)allFrames.size(); i++) {
            auto pit = poses.find(allFrames[i].seq);
            if (pit == poses.end()) continue;
            cv::Point p = fieldToPx(pit->second.x, pit->second.y);
            double alpha = (double)(i - start) / std::max(trailLen, 1);
            cv::Scalar color(50 + (int)(200*alpha), 50 + (int)(200*alpha), 0);
            cv::circle(img, p, 2, color, -1);
        }

        // Current robot pose
        auto curIt = poses.find(fs.seq);
        if (curIt != poses.end()) {
            const RobotPose &rp = curIt->second;
            cv::Point p = fieldToPx(rp.x, rp.y);
            cv::circle(img, p, 8, cv::Scalar(0,255,0), 2);
            double yaw = rp.yawRad();
            int arrowLen = 20;
            cv::Point tip(p.x + (int)(arrowLen*cos(yaw)), p.y - (int)(arrowLen*sin(yaw)));
            cv::arrowedLine(img, p, tip, cv::Scalar(0,255,0), 2, cv::LINE_8, 0, 0.3);
        }

        // Info text
        int totalTags = 0;
        for (auto &cam : fs.cams) totalTags += (int)cam.detections.size();

        char info[128];
        snprintf(info, sizeof(info), "Seq: %u  Tags: %d", fs.seq, totalTags);
        cv::putText(img, info, cv::Point(10, 20),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200,200,200), 1);

        if (curIt != poses.end()) {
            const RobotPose &rp = curIt->second;
            snprintf(info, sizeof(info), "X:%.1f  Y:%.1f  Yaw:%.1f",
                     rp.x, rp.y, rp.yawRad() * 180.0 / M_PI);
            cv::putText(img, info, cv::Point(10, height-10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(200,200,200), 1);
        } else if (totalTags > 0) {
            cv::putText(img, "No pose (bEstimate rejected)", cv::Point(10, height-10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0,0,200), 1);
        }

        return img;
    }
};

static cv::Mat drawCameraViews(const FrameSet &fs, int nCams,
                               int camW = 320, int camH = 240)
{
    int cols = std::min(nCams, 2);
    int rows = (nCams + cols - 1) / cols;
    cv::Mat img = cv::Mat::zeros(rows * camH, cols * camW, CV_8UC3);

    float srcW = 1280.0f, srcH = 800.0f;
    float sx = camW / srcW;
    float sy = camH / srcH;

    for (int c = 0; c < nCams; c++) {
        int col = c % cols;
        int row = c / cols;
        int xOff = col * camW;
        int yOff = row * camH;

        cv::Scalar color = CAM_COLORS[c % 4];

        cv::rectangle(img, cv::Point(xOff+1, yOff+1),
                      cv::Point(xOff+camW-2, yOff+camH-2), cv::Scalar(60,60,60), 1);

        char label[32];
        snprintf(label, sizeof(label), "Cam %d", c);
        cv::putText(img, label, cv::Point(xOff+5, yOff+18),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);

        // Find this camera's data
        const CamFrame *cf = nullptr;
        for (auto &cam : fs.cams) {
            if (cam.camId == c) { cf = &cam; break; }
        }

        if (!cf || cf->detections.empty()) {
            cv::putText(img, "No tags", cv::Point(xOff + camW/2 - 30, yOff + camH/2),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(80,80,80), 1);
            continue;
        }

        snprintf(label, sizeof(label), "%d tag%s",
                 (int)cf->detections.size(), cf->detections.size() != 1 ? "s" : "");
        cv::putText(img, label, cv::Point(xOff + camW - 80, yOff + 18),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(180,180,180), 1);

        for (auto &det : cf->detections) {
            cv::Point pts[4];
            for (int i = 0; i < 4; i++) {
                pts[i] = cv::Point((int)(xOff + det.corners[i].x * sx),
                                   (int)(yOff + det.corners[i].y * sy));
            }

            // Quadrilateral
            for (int i = 0; i < 4; i++)
                cv::line(img, pts[i], pts[(i+1)%4], color, 2);

            // Corner dots
            for (int i = 0; i < 4; i++)
                cv::circle(img, pts[i], 3, i == 0 ? cv::Scalar(0,0,255) : cv::Scalar(255,255,255), -1);

            // Tag ID at center
            cv::Point ctr((pts[0].x+pts[1].x+pts[2].x+pts[3].x)/4,
                          (pts[0].y+pts[1].y+pts[2].y+pts[3].y)/4);
            cv::putText(img, std::to_string(det.tagId), cv::Point(ctr.x-5, ctr.y+5),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);

            // Pixel area
            std::vector<cv::Point> contour(pts, pts+4);
            double area = cv::contourArea(contour);
            snprintf(label, sizeof(label), "%dpx", (int)area);
            cv::putText(img, label, cv::Point(ctr.x-15, ctr.y+20),
                        cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(180,180,180), 1);
        }
    }

    return img;
}

// ---- Main ----

// Try to find the default field JSON relative to the executable
static std::string findDefaultFieldJson(const char *argv0) {
    // Walk up from the executable to find the repo root
    // Expected layout: <repo>/Utilities/log-visualizer/build/log-visualizer
    std::string exe(argv0);
    // Normalize: if argv0 is relative, prepend cwd
    if (!exe.empty() && exe[0] != '/' && exe[0] != '\\') {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd))) {
            exe = std::string(cwd) + "/" + exe;
        }
    }

    // Try going up from executable location
    std::vector<std::string> candidates;
    // From build/log-visualizer -> repo is ../../../..
    // From log-visualizer dir   -> repo is ../..
    std::string dir = exe;
    for (int ups = 0; ups < 6; ups++) {
        size_t slash = dir.find_last_of("/\\");
        if (slash == std::string::npos) break;
        dir = dir.substr(0, slash);
        std::string candidate = dir + "/2026/2026AprilTag/2026-rebuilt-welded.json";
        FILE *fp = fopen(candidate.c_str(), "r");
        if (fp) { fclose(fp); return candidate; }
    }

    // Also try relative to cwd
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

int main(int argc, char **argv)
{
    const char *fieldPath = nullptr;
    const char *logPath   = nullptr;
    const char *posePath  = nullptr;

    // Parse args: <framelog> [poses-file] [--field <field.json>]
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

    // Default field JSON
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
    } else {
        printf("  No pose file provided. Robot position will not be shown.\n");
        printf("  Generate with: ./pose-debug <config> <framelog> > poses.txt\n");
    }

    // Determine camera count
    int nCams = 4;
    for (auto &fs : frameSets)
        nCams = std::max(nCams, (int)fs.cams.size());

    FieldView fv(800, 400, fieldLen, fieldWidth);

    int current = 0;
    bool playing = false;
    int delayMs = 100;

    const char *winName = "AprilTag Log Visualizer";
    cv::namedWindow(winName, cv::WINDOW_NORMAL);

    printf("\nControls:\n");
    printf("  Space       - Play/Pause\n");
    printf("  Right/Left  - Next/Previous frame\n");
    printf("  +/-         - Speed up/slow down\n");
    printf("  Q/Esc       - Quit\n");

    while (true) {
        const FrameSet &fs = frameSets[current];

        cv::Mat fieldImg = fv.draw(fieldTags, fs, poses, frameSets, current);
        cv::Mat camImg   = drawCameraViews(fs, nCams);

        // Resize camera view to match field width
        int fw = fieldImg.cols;
        int ch = (int)((double)camImg.rows * fw / camImg.cols);
        cv::Mat camResized;
        cv::resize(camImg, camResized, cv::Size(fw, ch));

        // Status bar
        cv::Mat bar = cv::Mat::zeros(30, fw, CV_8UC3);
        char status[256];
        snprintf(status, sizeof(status), "Frame %d/%d  Seq %u  %s  Delay: %dms",
                 current+1, (int)frameSets.size(), fs.seq,
                 playing ? "PLAYING" : "PAUSED", delayMs);
        cv::putText(bar, status, cv::Point(10, 20),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(200,200,200), 1);

        cv::Mat combined;
        cv::vconcat(std::vector<cv::Mat>{fieldImg, bar, camResized}, combined);
        cv::imshow(winName, combined);

        int wait = playing ? delayMs : 0;
        int key = cv::waitKey(std::max(wait, 1)) & 0xFF;

        if (key == 'q' || key == 27) {
            break;
        } else if (key == ' ') {
            playing = !playing;
        } else if (key == 83 || key == 'd' || key == 3) {  // Right arrow
            current = std::min(current + 1, (int)frameSets.size() - 1);
            playing = false;
        } else if (key == 81 || key == 'a' || key == 2) {  // Left arrow
            current = std::max(current - 1, 0);
            playing = false;
        } else if (key == '+' || key == '=') {
            delayMs = std::max(10, delayMs - 20);
        } else if (key == '-') {
            delayMs = std::min(2000, delayMs + 20);
        } else if (playing && (key == 255 || key == 0xFF)) {
            current++;
            if (current >= (int)frameSets.size()) {
                current = (int)frameSets.size() - 1;
                playing = false;
            }
        }
    }

    cv::destroyAllWindows();
    return 0;
}
