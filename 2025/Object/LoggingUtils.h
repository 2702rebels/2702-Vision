#pragma once
#include <spdlog/spdlog.h>

using namespace spdlog;

/** Default logger to print to console screen */
std::shared_ptr<logger> textLogsOutput;
void SetupTextLogger();
std::shared_ptr<logger> SetupFrameLogger(const char* name);