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
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/" + GetCurTimeStr() + "_" + "stdout.log", true);
	console_sink->set_pattern("[%T] [%^%l%$] %v");
	console_sink->set_pattern("[%Y-%m-%d %T] [%l] %v");
	std::vector<sink_ptr> sinks{ console_sink, file_sink };
	auto threadpool = std::make_shared<details::thread_pool>(16384, 1);
	textLogsOutput = std::make_shared<async_logger>(
		"ObjDetection",
		sinks.begin(),
		sinks.end(),
		// allow for 16384 messages in async queue, 1 async logging thread
		std::weak_ptr<details::thread_pool>(threadpool),
		spdlog::async_overflow_policy::block
		// if async queue does get overloaded, we should probably just increase the size of it, we're less constricted on ram than cpu
		// but data integrity is paramount, so block if it does somehow happen.
	);
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