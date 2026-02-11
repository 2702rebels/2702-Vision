#pragma once
#include <spdlog/spdlog.h>

using namespace spdlog;

/** @brief Default logger to print to console screen */
extern std::shared_ptr<logger> textLogsOutput;
/** @brief Function to instantiate @ref textLogsOutput and make it the default logger
 * @details Should be called in the main of every executable very early
 */
void SetupTextLogger();