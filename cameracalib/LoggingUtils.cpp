#include "LoggingUtils.h"
#include <iomanip>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>

std::shared_ptr<spdlog::logger> textLogsOutput;

std::string GetCurTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&timeNow);

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void SetupTextLogger() {
    // Create a global thread pool (must be done once)
    spdlog::init_thread_pool(16384, 2);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "Logs/" + GetCurTimeStr() + "_stdout.log", true);

    console_sink->set_pattern("[%Y-%m-%d %T] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

    auto logger = std::make_shared<spdlog::async_logger>(
        "ObjDetection",
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),   // <-- uses the global pool
        spdlog::async_overflow_policy::block
    );

    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
}