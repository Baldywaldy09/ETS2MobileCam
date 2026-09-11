/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: scs_logging.h
/*-----------------------------------------*/

#pragma once
#include <string>
#include <vector>
#include <scs_sdk/scssdk_telemetry.h>
#include <cstdarg>

namespace scs_logging
{
	inline std::string project_name;
	inline bool initialized{};
	inline scs_log_t game_logger{};

	inline void init(const scs_telemetry_init_params_t* params, const std::string& name)
	{
		if (initialized) return;
		project_name = name;

		const scs_telemetry_init_params_v101_t* version_params = reinterpret_cast<const scs_telemetry_init_params_v101_t*>(params);
		game_logger = version_params->common.log;

		initialized = true;
	}

	inline void scs_log(scs_log_type_t log_type, const char* format, ...)
	{
		if (!initialized) return;

		va_list args;
		va_start(args, format);

		std::vector<char> buffer(1024);
		const int len = std::vsnprintf(buffer.data(), buffer.size(), format, args);
		if (len < 0)
		{
			va_end(args);
			return;
		}

		if (static_cast<size_t>(len) >= buffer.size())
		{
			buffer.resize(len + 1);
			std::vsnprintf(buffer.data(), buffer.size(), format, args);
		}

		va_end(args);

		game_logger(log_type, ("[" + project_name + "] " + buffer.data()).c_str());
	}

	inline void shutdown()
	{
		scs_log(0, "Plugin Unloaded");
	}
}
