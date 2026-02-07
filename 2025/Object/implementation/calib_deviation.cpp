#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <sstream>
#include <LoggingUtils.h>
using namespace nlohmann;
namespace fs = std::filesystem;
using namespace spdlog;

double variance(const std::vector<double>& v) {
    if (v.empty()) return 0.0;

    double mean = 0.0;
    for (double x : v) mean += x;
    mean /= v.size();

    double var = 0.0;
    for (double x : v) {
        double diff = x - mean;
        var += diff * diff;
    }

    return var / v.size();
}

double average(const std::vector<double>& v)
{
    double sum = 0;
    for (double x : v) sum += x;
    return sum / v.size();
}

int main()
{
    SetupTextLogger();
    std::vector<fs::path> configPaths;
    while (true)
    {
        std::string curConfigPath;
        std::cout << "Enter a path of a config file to use(null to finish): ";
        std::getline(std::cin, curConfigPath);
        if (curConfigPath == "null")
        {
            break;
        }
        if (!fs::exists(curConfigPath))
        {
            std::cout << "Config path" << curConfigPath << "does not exist, skipping";
            continue;
        }
        configPaths.push_back(curConfigPath);
    }
    std::vector<json> configs;
    for (const fs::path& path : configPaths)
    {
        std::ifstream config(path);
        std::stringstream ss;
        ss << config.rdbuf();
        configs.push_back(json::parse(ss.str()));
    }
    std::vector<double> rmsErrors;
    std::vector<std::vector<double>> cameraMatrices;
    std::vector<std::vector<double>> distortionCoeffs;
    for (const json& cfg : configs)
    {
        rmsErrors.push_back(cfg["rms_error"]);
        cameraMatrices.push_back(cfg["camera_matrix"]);
        distortionCoeffs.push_back(cfg["distortion_coeffs"]);
    }
    double rmsVariance = variance(rmsErrors);
    double avgRmsError = average(rmsErrors);
    info("variance in rms err: {}, avg rms err: {}", rmsVariance, avgRmsError);
    for (int i = 0; i < cameraMatrices[0].size(); i++)
    {
        std::vector<double> curValueVec;
        for (const std::vector<double>& matrix : cameraMatrices)
        {
            curValueVec.push_back(matrix[i]);
        }
        double valueVariance = variance(curValueVec);
        double valueAverage = average(curValueVec);
        info("Cam matrix param {}: variance: {}, average: {}", i, valueVariance, valueAverage);
    }
    for (int i = 0; i < distortionCoeffs[0].size(); i++)
    {
        std::vector<double> curValueVec;
        for (const std::vector<double>& matrix : distortionCoeffs)
        {
            curValueVec.push_back(matrix[i]);
        }
        double valueVariance = variance(curValueVec);
        double valueAverage = average(curValueVec);
        info("Distortion coeffs param {}: variance: {}, average: {}", i, valueVariance, valueAverage);
    }
}