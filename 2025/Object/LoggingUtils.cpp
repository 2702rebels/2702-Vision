#include "LoggingUtils.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>

std::string GetCurTimeStr() {
	auto now = std::chrono::system_clock::now();
	std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
	std::tm localTime = *std::localtime(&timeNow);

	std::ostringstream oss;
	oss << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
	return oss.str();
}

void SetupTextLogger() {
	// Set up cout log
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/" + GetCurTimeStr() + "_" + "stdout.log", true);
	console_sink->set_pattern("[%T] [%^%l%$] %v");
	console_sink->set_pattern("[%Y-%m-%d %T] [%l] %v");
	std::vector<sink_ptr> sinks{ console_sink, file_sink };
	textLogsOutput = create_async(("ObjDetection", sinks.begin(), sinks.end());
	register_logger(textLogsOutput);
	set_default_logger(textLogsOutput);
}

std::shared_ptr<logger> SetupFrameLogger(const char* name) {
	auto logger = spdlog::create_async<sinks::basic_file_sink_mt>(
		name, "Logs/" + GetCurTimeStr() + "_" + name + ".log"
	);
	register_logger(logger);
	return logger;
}