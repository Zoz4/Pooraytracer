#pragma once
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <glm/gtx/string_cast.hpp>
#include <chrono>

#define RELATIVE_FILE(path) (path + sizeof(PROJECT_ROOT) - 1)
#define LOGGER_FORMAT "[%^%l%$] %v"

#define LOGD(...) SPDLOG_DEBUG(__VA_ARGS__);
#define LOGI(...) SPDLOG_INFO(__VA_ARGS__);
#define LOGW(...) SPDLOG_WARN(__VA_ARGS__);
#define LOGE(...) SPDLOG_ERROR("[{}:{}] {}", RELATIVE_FILE(__FILE__), __LINE__, fmt::format(__VA_ARGS__));

inline std::string GetTimestamp()
{
	auto now = std::chrono::system_clock::now();
	std::time_t time_now = std::chrono::system_clock::to_time_t(now);
	std::tm* tm_now = std::localtime(&time_now);
	std::stringstream ss;

	ss << std::setfill('0')
		<< std::setw(4) << (tm_now->tm_year + 1900) << ""
		<< std::setw(2) << (tm_now->tm_mon + 1) << ""
		<< std::setw(2) << tm_now->tm_mday << "_"
		<< std::setw(2) << tm_now->tm_hour << ""
		<< std::setw(2) << tm_now->tm_min;

	return ss.str();
}

inline std::string GetExecutionTimeInMinutes(const std::chrono::steady_clock::time_point& start) {
	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	double seconds = duration.count() / 1000.0;
	std::stringstream ss;
	ss << std::fixed << std::setprecision(2) << seconds <<"s";
	return ss.str();
}