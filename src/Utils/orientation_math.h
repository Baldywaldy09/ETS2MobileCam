/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Utils/orientation_math.h
/*-----------------------------------------*/

#pragma once
#include <cmath>

#include "Prism3D/Containers/math.h"

namespace phone_math
{
	[[nodiscard]] inline prism::quat_t quat_from_axis_angle(double ax, double ay, double az, double angle_rad)
	{
		const double half = angle_rad * 0.5;
		const double s = std::sin(half);

		prism::quat_t q{};
		q.w = static_cast<float>(std::cos(half));
		q.x = static_cast<float>(ax * s);
		q.y = static_cast<float>(ay * s);
		q.z = static_cast<float>(az * s);
		return q;
	}

	[[nodiscard]] inline prism::quat_t quat_multiply(const prism::quat_t& a, const prism::quat_t& b)
	{
		prism::quat_t r{};
		r.w = static_cast<float>(a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
		r.x = static_cast<float>(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y);
		r.y = static_cast<float>(a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x);
		r.z = static_cast<float>(a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w);
		return r;
	}

	[[nodiscard]] inline prism::quat_t quat_conjugate(const prism::quat_t& q)
	{
		return prism::quat_t{ q.w, -q.x, -q.y, -q.z };
	}

	// Spherical linear interpolation, shortest path; falls back to a linear blend near-parallel
	[[nodiscard]] inline prism::quat_t quat_slerp(const prism::quat_t& a, const prism::quat_t& b_in, float t)
	{
		prism::quat_t b = b_in;
		float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;

		if (dot < 0.0f)
		{
			b = { -b.w, -b.x, -b.y, -b.z };
			dot = -dot;
		}

		if (dot > 0.9995f)
		{
			prism::quat_t r{
				a.w + t * (b.w - a.w),
				a.x + t * (b.x - a.x),
				a.y + t * (b.y - a.y),
				a.z + t * (b.z - a.z)
			};
			const float len = std::sqrt(r.w * r.w + r.x * r.x + r.y * r.y + r.z * r.z);
			if (len > 1e-6f) { r.w /= len; r.x /= len; r.y /= len; r.z /= len; }
			return r;
		}

		const float theta0 = std::acos(dot);
		const float theta = theta0 * t;
		const float sin_theta0 = std::sin(theta0);
		const float s0 = std::cos(theta) - dot * std::sin(theta) / sin_theta0;
		const float s1 = std::sin(theta) / sin_theta0;

		return prism::quat_t{
			s0 * a.w + s1 * b.w,
			s0 * a.x + s1 * b.x,
			s0 * a.y + s1 * b.y,
			s0 * a.z + s1 * b.z
		};
	}

	[[nodiscard]] inline prism::float3_t rotate_vector(const prism::quat_t& q, const prism::float3_t& v)
	{
		const prism::quat_t qv{ 0.0f, v.x, v.y, v.z };
		const prism::quat_t r = quat_multiply(quat_multiply(q, qv), quat_conjugate(q));
		return prism::float3_t{ r.x, r.y, r.z };
	}

	[[nodiscard]] inline prism::float3_t normalize(const prism::float3_t& v)
	{
		const double len = std::sqrt(static_cast<double>(v.x) * v.x + static_cast<double>(v.y) * v.y + static_cast<double>(v.z) * v.z);
		if (len < 1e-6) return prism::float3_t{ 0.0f, 0.0f, 0.0f };
		return prism::float3_t{
			static_cast<float>(v.x / len),
			static_cast<float>(v.y / len),
			static_cast<float>(v.z / len)
		};
	}

	// Device-local-axes -> engine-axes change of basis, currently identity
	inline const prism::quat_t DEVICE_TO_ENGINE_BASIS = prism::quat_t{ 1.0f, 0.0f, 0.0f, 0.0f };

	[[nodiscard]] inline prism::quat_t device_quat_to_engine(const prism::quat_t& device_relative)
	{
		return quat_multiply(quat_multiply(DEVICE_TO_ENGINE_BASIS, device_relative), quat_conjugate(DEVICE_TO_ENGINE_BASIS));
	}
}
