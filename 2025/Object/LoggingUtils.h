#include <spdlog/spdlog.h>

using namespace spdlog;

/** Default logger to print to console screen */
std::unique_ptr<logger> textLogsOutput;
void SetupTextLogger();
std::unique_ptr<logger> SetupFrameLogger(const char* name);