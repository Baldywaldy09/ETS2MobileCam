/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Utils/base64.h
/*-----------------------------------------*/

#pragma once
#include <cstdint>
#include <string>

namespace base64
{
	[[nodiscard]] inline std::string encode(const uint8_t* data, size_t length)
	{
		static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		std::string out;
		out.reserve(((length + 2) / 3) * 4);

		size_t i = 0;
		for (; i + 3 <= length; i += 3)
		{
			const uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
			out.push_back(table[(n >> 18) & 0x3F]);
			out.push_back(table[(n >> 12) & 0x3F]);
			out.push_back(table[(n >> 6) & 0x3F]);
			out.push_back(table[n & 0x3F]);
		}

		const size_t remaining = length - i;
		if (remaining == 1)
		{
			const uint32_t n = data[i] << 16;
			out.push_back(table[(n >> 18) & 0x3F]);
			out.push_back(table[(n >> 12) & 0x3F]);
			out.push_back('=');
			out.push_back('=');
		}
		else if (remaining == 2)
		{
			const uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
			out.push_back(table[(n >> 18) & 0x3F]);
			out.push_back(table[(n >> 12) & 0x3F]);
			out.push_back(table[(n >> 6) & 0x3F]);
			out.push_back('=');
		}

		return out;
	}
}
