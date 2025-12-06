#pragma once
#include <spdlog/spdlog.h>

using namespace spdlog;

/** Default logger to print to console screen */
extern std::shared_ptr<logger> textLogsOutput;
void SetupTextLogger();