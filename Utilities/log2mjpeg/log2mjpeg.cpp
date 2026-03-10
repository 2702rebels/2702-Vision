#include "Bayer2Grey.h"
#include "Bayer2RGB.h"
#include "VisionLog.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <vector>

enum class pmode : std::uint8_t {
    pmcolor,
    pmcolor2,
    pmgrey,
    pmgrey2,
    pmbayer,
    pmgreen,
    pmred,
    pmblue,
    pminput
};

namespace fs = std::filesystem;

static pmode procmode = pmode::pminput;

static void Bayer2Red(cv::Mat& matRed, const cv::Mat& matBayer) {
    assert(matRed.rows == matBayer.rows / 2 && matRed.cols == matBayer.cols / 2);
    assert(matRed.type() == CV_8UC1 && matBayer.type() == CV_8UC1);
    assert((matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0);

    for (int r = 0; r < matRed.rows; r++) {
        const uint8_t* pu8Bayer = matBayer.ptr(r * 2) + 1;
        uint8_t* pu8Red = matRed.ptr(r);
        for (int c = 0; c < matRed.cols; c++) {
            *pu8Red++ = *pu8Bayer;
            pu8Bayer += 2;
        }
    }
}

static void Bayer2Blue(cv::Mat& matBlue, const cv::Mat& matBayer) {
    assert(matBlue.rows == matBayer.rows / 2 &&
           matBlue.cols == matBayer.cols / 2);
    assert(matBlue.type() == CV_8UC1 && matBayer.type() == CV_8UC1);
    assert((matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0);

    for (int r = 0; r < matBlue.rows; r++) {
        const uint8_t* pu8Bayer = matBayer.ptr(r * 2 + 1);
        uint8_t* pu8Blue = matBlue.ptr(r);
        for (int c = 0; c < matBlue.cols; c++) {
            *pu8Blue++ = *pu8Bayer;
            pu8Bayer += 2;
        }
    }
}

static void Bayer2Green(cv::Mat& matGreen, const cv::Mat& matBayer) {
    assert(matGreen.rows == matBayer.rows / 2 &&
           matGreen.cols == matBayer.cols / 2);
    assert(matGreen.type() == CV_8UC1 && matBayer.type() == CV_8UC1);
    assert((matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0);

    int nColsPlus1 = matBayer.cols + 1;
    for (int r = 0; r < matGreen.rows; r++) {
        const uint8_t* pu8Bayer = matBayer.ptr(r * 2);
        uint8_t* pu8Green = matGreen.ptr(r);
        for (int c = 0; c < matGreen.cols; c++) {
            *pu8Green++ = (pu8Bayer[0] + pu8Bayer[nColsPlus1]) / 2;
            pu8Bayer += 2;
        }
    }
}

static void Usage() {
    std::cerr << "Usage: log2mjpeg "
                 "[-colour|colour2|grey|grey2|bayer|red|green|blue|input] infile "
                 "outdir\n";
    std::cerr << "      colour     -> RGB\n";
    std::cerr << "      colour2    -> RGB 1/2 resolution\n";
    std::cerr << "      grey       -> GreyScale\n";
    std::cerr << "      grey2      -> GreyScale 1/2 resolution\n";
    std::cerr << "      beyer      -> Raw Bayer frame\n";
    std::cerr << "      red        -> Extracts the red pixels\n";
    std::cerr << "      green      -> Extracts the green pixels\n";
    std::cerr << "      blue       -> Extracts the blue pixels\n";
    exit(1);
}

int main(int argc, char** argv) {
    const char* pszIn = nullptr;
    const char* pszOut = nullptr;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-colour") == 0) {
                procmode = pmode::pmcolor;
            } else if (strcmp(argv[i], "-colour2") == 0) {
                procmode = pmode::pmcolor2;
            } else if (strcmp(argv[i], "-grey") == 0) {
                procmode = pmode::pmgrey;
            } else if (strcmp(argv[i], "-grey2") == 0) {
                procmode = pmode::pmgrey2;
            } else if (strcmp(argv[i], "-bayer") == 0) {
                procmode = pmode::pmbayer;
            } else if (strcmp(argv[i], "-red") == 0) {
                procmode = pmode::pmred;
            } else if (strcmp(argv[i], "-green") == 0) {
                procmode = pmode::pmgreen;
            } else if (strcmp(argv[i], "-blue") == 0) {
                procmode = pmode::pmblue;
            } else if (strcmp(argv[i], "-input") == 0) {
                procmode = pmode::pminput;
            } else {
                Usage();
}
        } else if (pszIn == nullptr) {
            pszIn = argv[i];
        } else if (pszOut == nullptr) {
            pszOut = argv[i];
        } else {
            Usage();
}
    }
    if (pszOut == nullptr) {
        Usage();
        return 1;
    }

    VisionLogReader vl(pszIn);
    if (!fs::exists(pszOut)) {
        fs::create_directories(pszOut);
    }

    int nSeq = 0;
    std::optional<cv::Mat> img;
    while ((img = vl.TryReadFrame())) {
        if (!img.has_value()) {
            break;
        }
        cv::Mat imgOut;
        switch (procmode) {
        case pmode::pminput:
            imgOut = img.value();
            break;
        case pmode::pmgrey: {
            cv::Mat matGrey(img.value().rows, img.value().cols, CV_8UC1);
            Bayer2Grey(matGrey, img.value());
            imgOut = matGrey;
        } break;
        case pmode::pmgrey2: {
            cv::Mat matGrey(img.value().rows / 2, img.value().cols / 2, CV_8UC1);
            Bayer2HalfGrey(matGrey, img.value());
            imgOut = matGrey;
        } break;
        case pmode::pmbayer:
            imgOut = img.value();
            break;
        case pmode::pmred: {
            cv::Mat matRed(img.value().rows / 2, img.value().cols / 2, CV_8UC1);
            Bayer2Red(matRed, img.value());
            imgOut = matRed;
        } break;
        case pmode::pmgreen: {
            cv::Mat matGreen(img.value().rows / 2, img.value().cols / 2, CV_8UC1);
            Bayer2Green(matGreen, img.value());
            imgOut = matGreen;
        } break;
        case pmode::pmblue: {
            cv::Mat matBlue(img.value().rows / 2, img.value().cols / 2, CV_8UC1);
            Bayer2Blue(matBlue, img.value());
            imgOut = matBlue;
        } break;
        case pmode::pmcolor2: {
            cv::Mat matRGB(img.value().rows / 2, img.value().cols / 2, CV_8UC3);
            Bayer2HalfRGB(matRGB, img.value());
            imgOut = matRGB;
        } break;
        case pmode::pmcolor:
        default: {
            cv::Mat matRGB(img.value().rows, img.value().cols, CV_8UC3);
            Bayer2RGB(matRGB, img.value());
            imgOut = matRGB;
        } break;
        }
        std::vector<uchar> vJPEG;
        cv::imencode(".jpg", imgOut, vJPEG);
        std::ofstream outfile(std::string(pszOut) + "/" + std::to_string(nSeq) +
                              ".jpg");
        outfile.write(reinterpret_cast<const char*>(vJPEG.data()), vJPEG.size()); //NOLINT
        nSeq++;
    }
    return 0;
}
