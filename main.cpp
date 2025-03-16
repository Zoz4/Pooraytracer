#include "Source/Logger.h"
#include "Source/RandomNumberGenerator.h"

int main(void)
{
	//spdlog::set_pattern(LOGGER_FORMAT);
	spdlog::set_level(spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL));

	LOGI("Hello Pooraytracer!");
	for (int i = 0; i < 10; ++i) {
		LOGI("i:{}", Pooraytracer::RandomFloat());
	}

	return 0;
}