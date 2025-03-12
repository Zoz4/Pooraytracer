#pragma once
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#define RELATIVE_FILE(path) (path + sizeof(PROJECT_ROOT) - 1)
#define LOGGER_FORMAT "[%^%l%$] %v"

#define LOGD(...) SPDLOG_DEBUG(__VA_ARGS__);
#define LOGI(...) SPDLOG_INFO(__VA_ARGS__);
#define LOGW(...) SPDLOG_WARN(__VA_ARGS__);
#define LOGE(...) SPDLOG_ERROR("[{}:{}] {}", RELATIVE_FILE(__FILE__), __LINE__, fmt::format(__VA_ARGS__));