/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Net/CameraInput.h
/*-----------------------------------------*/

#pragma once
#include <mutex>

#include "Prism3D/Containers/math.h"
#include "Utils/orientation_math.h"

struct camera_input_state_t
{
	prism::quat_t rotation{ 1.f, 0.f, 0.f, 0.f };
	prism::float3_t joystick{ 0.f, 0.f, 0.f }; // x = strafe, y = forward/back, z = up/down
	float fov{ -1.f };
	bool control_enabled{};
	bool connected{};
	bool portrait_mode{};
	bool free_camera_active{}; // in game camera

	// User-adjustable (web settings page)
	float move_speed{ 2.5f };
	float walk_accel_time{ 0.2f };
	float zoom_time_constant{ 0.18f };
	float stabilizer_tau_max{ 0.18f };
	float cast_fps{ 12.0f };
	float head_bob_strength{ 1.0f };
	float cast_quality{ 70.0f }; // JPEG quality, 1-100
	float cast_resolution{ 720.0f }; // long edge cap, px
	float idle_sway_strength{ 1.0f };
};

// Shared state written by the phone websocket thread and read once per frame by telemetry_tick()
class CameraInput
{
public:
	static CameraInput* get()
	{
		static CameraInput instance;
		return &instance;
	}

	void set_device_rotation(const prism::quat_t& rotation)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_device_rotation = rotation;
		recompute_rotation();
	}

	void set_trim(const prism::quat_t& trim)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_trim_rotation = trim;
		recompute_rotation();
	}

	void set_joystick(const prism::float3_t& joystick)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.joystick = joystick;
	}

	void set_fov(float fov)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.fov = fov;
	}

	void set_settings(float move_speed, float walk_accel_time, float zoom_time_constant, float stabilizer_tau_max,
		float cast_fps, float head_bob_strength, float cast_quality, float cast_resolution, float idle_sway_strength)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.move_speed = move_speed;
		m_state.walk_accel_time = walk_accel_time;
		m_state.zoom_time_constant = zoom_time_constant;
		m_state.stabilizer_tau_max = stabilizer_tau_max;
		m_state.cast_fps = cast_fps;
		m_state.head_bob_strength = head_bob_strength;
		m_state.cast_quality = cast_quality;
		m_state.cast_resolution = cast_resolution;
		m_state.idle_sway_strength = idle_sway_strength;
	}

	void set_portrait_mode(bool enabled)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.portrait_mode = enabled;
	}

	void set_free_camera_active(bool active)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.free_camera_active = active;
	}

	void set_control_enabled(bool enabled)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.control_enabled = enabled;
	}

	void set_connected(bool connected)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_state.connected = connected;

		if (!connected)
		{
			m_state.control_enabled = false;
			m_state.joystick = { 0.f, 0.f, 0.f };
		}
	}

	[[nodiscard]] camera_input_state_t get_state() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_state;
	}

private:
	CameraInput() = default;
	CameraInput(const CameraInput&) = delete;
	CameraInput& operator=(const CameraInput&) = delete;

	// Caller must hold m_mutex
	void recompute_rotation()
	{
		m_state.rotation = phone_math::quat_multiply(m_trim_rotation, m_device_rotation);
	}

	mutable std::mutex m_mutex;
	camera_input_state_t m_state;
	prism::quat_t m_device_rotation{ 1.f, 0.f, 0.f, 0.f };
	prism::quat_t m_trim_rotation{ 1.f, 0.f, 0.f, 0.f };
};
