/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Prism3D/Containers/math.h
/*-----------------------------------------*/

#pragma once
#include <cstdint>
#include <cmath>
#include <string.h>
#include <algorithm>

// last updated 1.?? old

#pragma pack(push, 1)
namespace prism
{
	struct float3_t {
		float x, y, z;
	};

	struct float2_t {
		float x, y;
	};

	struct quat_t {
		float w, x, y, z;
	};



	struct matrix4x4_t {
		float m1, m2, m3, m4;
		float m5, m6, m7, m8;
		float m9, m10, m11, m12;
		float m13, m14, m15, m16;

		float* operator[](int row) {
			return &m1 + row * 4;
		}
		const float* operator[](int row) const {
			return &m1 + row * 4;
		}
	};



	// Chunk size 512
	class placement_t // Size: 0x0020
	{
	public:
		prism::float3_t position; //0x0000 (0x0c)
		int16_t chunk_x;          //0x000C (0x02)
		int16_t chunk_z;          //0x000E (0x02)
		prism::quat_t rotation;   //0x0010 (0x10)
	};
	static_assert(sizeof(placement_t) == 0x20);

};
#pragma pack(pop)
