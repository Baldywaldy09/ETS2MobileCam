/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Utils/sha1.h
/*-----------------------------------------*/

#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

// Only used for the websocket handshake's Sec-WebSocket-Accept, this exists purely to avoid pulling in OpenSSL for one hash
namespace sha1
{
	[[nodiscard]] inline std::array<uint8_t, 20> digest(const std::string& input)
	{
		uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;

		std::string message = input;
		const uint64_t bit_length = static_cast<uint64_t>(message.size()) * 8;

		message.push_back(static_cast<char>(0x80));
		while (message.size() % 64 != 56) message.push_back(static_cast<char>(0x00));

		for (int i = 7; i >= 0; --i)
			message.push_back(static_cast<char>((bit_length >> (i * 8)) & 0xFF));

		for (size_t chunk_start = 0; chunk_start < message.size(); chunk_start += 64)
		{
			uint32_t w[80]{};
			for (int i = 0; i < 16; ++i)
			{
				w[i] = (static_cast<uint8_t>(message[chunk_start + i * 4]) << 24) |
					(static_cast<uint8_t>(message[chunk_start + i * 4 + 1]) << 16) |
					(static_cast<uint8_t>(message[chunk_start + i * 4 + 2]) << 8) |
					(static_cast<uint8_t>(message[chunk_start + i * 4 + 3]));
			}

			for (int i = 16; i < 80; ++i)
			{
				const uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
				w[i] = (v << 1) | (v >> 31);
			}

			uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

			for (int i = 0; i < 80; ++i)
			{
				uint32_t f, k;
				if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
				else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
				else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
				else { f = b ^ c ^ d; k = 0xCA62C1D6; }

				const uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
				e = d;
				d = c;
				c = (b << 30) | (b >> 2);
				b = a;
				a = temp;
			}

			h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
		}

		std::array<uint8_t, 20> result{};
		uint32_t hs[5] = { h0, h1, h2, h3, h4 };
		for (int i = 0; i < 5; ++i)
		{
			result[i * 4 + 0] = static_cast<uint8_t>((hs[i] >> 24) & 0xFF);
			result[i * 4 + 1] = static_cast<uint8_t>((hs[i] >> 16) & 0xFF);
			result[i * 4 + 2] = static_cast<uint8_t>((hs[i] >> 8) & 0xFF);
			result[i * 4 + 3] = static_cast<uint8_t>(hs[i] & 0xFF);
		}
		return result;
	}
}
