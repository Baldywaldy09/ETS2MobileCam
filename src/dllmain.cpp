/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: dllmain.cpp
/*-----------------------------------------*/

#include <algorithm>
#include <chrono>
#include <cmath>

#include "bmem.h"
#include "globals.h"
#include "PCInput.h"
#include "scs_logging.h"
#include "Prism3D/Types/camera_manager.h"
using namespace scs_logging;

#include "Prism3D/Actors/camera_manager.h"
#include "Net/CameraInput.h"
#include "Net/PhoneServer.h"
#include "Utils/orientation_math.h"

namespace
{
	uintptr_t freecam_tick_function_address{};

	SCSAPI_VOID telemetry_tick(const scs_event_t event, const void* const event_info, scs_context_t context)
	{
		static auto last_tick = std::chrono::steady_clock::now();
		const auto now = std::chrono::steady_clock::now();
		const float dt = std::min(std::chrono::duration<float>(now - last_tick).count(), 0.1f);
		last_tick = now;

		// Base FOV captured on enable so the slider scales relative to it
		static bool control_was_enabled = false;
		static float base_h_fov = 0.0f;
		static float base_v_fov = 0.0f;

		static prism::quat_t smoothed_rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		static float smoothed_fov = 1.0f;
		static float motion_estimate_deg = 0.0f;

		// Movement
		static float vel_x = 0.0f;
		static float vel_z = 0.0f;
		static float vel_y = 0.0f;

		static float step_phase = 0.0f;
		static float idle_phase = 0.0f;
		static prism::float3_t walk_pos{ 0.0f, 0.0f, 0.0f };

		// Camera to restore on disable
		static uint32_t previous_camera_index = 0;
		static bool was_applying_control = false;
		static bool control_was_pc = false;
		static bool freecam_tick_patched = false;

		// PC walk mode (F9, see PCInput.h) - yaw-then-pitch mouse-look
		static float pc_yaw = 0.0f;
		static float pc_pitch = 0.0f;
		static float pc_tilt = 0.0f;
		static float pc_fov = 1.0f;

		if (prism::camera_manager_u* camera_manager = prism::camera_manager_u::get())
		{
			camera_input_state_t phone = CameraInput::get()->get_state();

			const bool pc_toggled = PCInput::get()->poll_toggle();
			bool pc_active = PCInput::get()->active();

			// Enabling one control source disables the other
			static bool phone_control_was_enabled = false;
			const bool phone_control_enabling_now = phone.control_enabled && !phone_control_was_enabled;
			phone_control_was_enabled = phone.control_enabled;

			if (phone_control_enabling_now && pc_active)
			{
				PCInput::get()->deactivate();
				pc_active = false;
			}
			else if (pc_toggled && pc_active)
			{
				CameraInput::get()->set_control_enabled(false);
				phone.control_enabled = false;
			}

			if (pc_toggled && pc_active && camera_manager->m_cameras.size >= 1)
			{
				// Seed yaw/pitch from the free camera's current facing, so mouse-look doesn't snap
				const prism::float3_t fwd = phone_math::rotate_vector(camera_manager->m_cameras[0]->m_placement.rotation, { 0.0f, 0.0f, -1.0f });
				pc_yaw = std::atan2(-fwd.x, -fwd.z);
				pc_pitch = std::asin(std::clamp(fwd.y, -1.0f, 1.0f));
			}

			if (pc_active)
			{
				phone.control_enabled = true;

				float mouse_dx = 0.0f, mouse_dy = 0.0f;
				PCInput::get()->poll_mouse_delta(mouse_dx, mouse_dy);

				float target_tilt = 0.0f;
				if (!pc_toggled)
				{
					// Skip the toggle-on frame's delta - the cursor just got warped to center
					constexpr float pitch_limit = PC_PITCH_LIMIT_DEG * PI / 180.0f;
					const float yaw_delta = -mouse_dx * PC_MOUSE_SENSITIVITY;
					pc_yaw += yaw_delta;
					pc_pitch = std::clamp(pc_pitch + -mouse_dy * PC_MOUSE_SENSITIVITY, -pitch_limit, pitch_limit);

					// Handheld lean into turns, proportional to turn rate, faded down while standing still
					constexpr float tilt_limit = PC_TILT_MAX_DEG * PI / 180.0f;
					const float yaw_rate = yaw_delta / std::max(dt, 0.001f);
					const float walk_speed_fraction = std::min(1.0f, std::sqrt(vel_x * vel_x + vel_z * vel_z) / std::max(phone.move_speed, 0.01f));
					const float tilt_gate = PC_TILT_STATIONARY_FRACTION + (1.0f - PC_TILT_STATIONARY_FRACTION) * walk_speed_fraction;
					target_tilt = std::clamp(-yaw_rate * PC_TILT_STRENGTH * tilt_gate, -tilt_limit, tilt_limit);
				}
				const float tilt_smoothing = 1.0f - std::exp(-dt / PC_TILT_TAU);
				pc_tilt += (target_tilt - pc_tilt) * tilt_smoothing;

				// Same yaw/pitch/roll axis convention as the trim system below; tilt composed as a roll
				phone.rotation = phone_math::quat_multiply(
					phone_math::quat_multiply(
						phone_math::quat_from_axis_angle(0.0, 1.0, 0.0, static_cast<double>(pc_yaw)),
						phone_math::quat_from_axis_angle(1.0, 0.0, 0.0, static_cast<double>(pc_pitch))),
					phone_math::quat_from_axis_angle(0.0, 0.0, 1.0, static_cast<double>(pc_tilt)));
				phone.joystick = PCInput::get()->poll_movement();

				pc_fov = PCInput::get()->poll_middle_click() ? 1.0f : std::clamp(pc_fov - PCInput::get()->poll_wheel_delta() * PC_ZOOM_STEP, PC_ZOOM_MIN, PC_ZOOM_MAX);
				phone.fov = pc_fov;
			}

			const bool on_free_camera = camera_manager->m_current_camera == 0 && camera_manager->m_cameras.size >= 1;

			// Something else moved us off the free camera without going through our disable path
			bool external_camera_change = false;
			if (was_applying_control && !on_free_camera && phone.control_enabled)
			{
				phone.control_enabled = false;
				CameraInput::get()->set_control_enabled(false);
				if (pc_active) PCInput::get()->deactivate();
				external_camera_change = true;
			}

			// m_target_camera is only a request the engine applies next tick, so these edges can be a frame ahead of the camera actually landing on the free camera
			const bool control_enabling_now = !control_was_enabled && phone.control_enabled;
			const bool control_disabling_now = control_was_enabled && !phone.control_enabled;

			if (control_enabling_now)
			{
				// Store whatever camera we were on so disabling can put it back
				previous_camera_index = camera_manager->m_current_camera;
				camera_manager->m_target_camera = 0;
				control_was_pc = pc_active;

				scs_log(0, control_was_pc ? "PC walk control enabled" : "Phone control enabled");
			}
			else if (control_disabling_now)
			{
				// Don't fight an external camera change by switching back to what we remembered
				if (!external_camera_change)
					camera_manager->m_target_camera = previous_camera_index;

				const char* source = control_was_pc ? "PC walk control" : "Phone control";
				if (external_camera_change)
					scs_log(0, "%s disabled (camera changed externally)", source);
				else
					scs_log(0, "%s disabled", source);
			}
			control_was_enabled = phone.control_enabled;

			CameraInput::get()->set_free_camera_active(on_free_camera);

			// Block the free cam tick function while we are controlling it
			const bool should_patch_freecam = on_free_camera && phone.control_enabled;
			if (should_patch_freecam && !freecam_tick_patched)
			{
				DWORD old;
				VirtualProtect(reinterpret_cast<void*>(freecam_tick_function_address), 2, PAGE_EXECUTE_READWRITE, &old);
				*reinterpret_cast<uint8_t*>(freecam_tick_function_address + 0) = 0xC3;
				VirtualProtect(reinterpret_cast<void*>(freecam_tick_function_address), 2, old, &old);
				freecam_tick_patched = true;
			}
			else if (!should_patch_freecam && freecam_tick_patched)
			{
				DWORD old;
				VirtualProtect(reinterpret_cast<void*>(freecam_tick_function_address), 2, PAGE_EXECUTE_READWRITE, &old);
				*reinterpret_cast<uint8_t*>(freecam_tick_function_address + 0) = 0x40;
				VirtualProtect(reinterpret_cast<void*>(freecam_tick_function_address), 2, old, &old);
				freecam_tick_patched = false;
			}

			if (on_free_camera)
			{
				prism::core_camera_u* freecam = camera_manager->m_cameras[0];

				// Camera can take a tick to actually land on the free camera after control turns on
				const bool applying_now = phone.control_enabled && !was_applying_control;
				if (applying_now)
				{
					base_h_fov = freecam->m_horizontal_fov;
					base_v_fov = freecam->m_vertical_fov;
					smoothed_rotation = phone.rotation;

					if (phone.fov > 0.0f)
						smoothed_fov = phone.fov;

					vel_x = 0.f;
					vel_y = 0.f;
					vel_z = 0.f;

					motion_estimate_deg = 0.f;
					step_phase = 0.f;
					idle_phase = 0.f;
					pc_tilt = 0.f;
					walk_pos = freecam->m_placement.position;
				}

				if (phone.control_enabled)
				{
					if (pc_active)
					{
						// Mouse input is already direct, no need for the phone gyro stabilizer
						smoothed_rotation = phone.rotation;
					}
					else
					{
						const float rotation_dot_raw = smoothed_rotation.w * phone.rotation.w
							+ smoothed_rotation.x * phone.rotation.x
							+ smoothed_rotation.y * phone.rotation.y
							+ smoothed_rotation.z * phone.rotation.z;
						const float rotation_dot = std::min(1.0f, std::abs(rotation_dot_raw));
						const float rotation_angle_deg = 2.0f * std::acos(rotation_dot) * (180.0f / PI);

						constexpr float motion_estimate_tau = 0.08f;
						const float motion_smoothing = 1.0f - std::exp(-dt / motion_estimate_tau);
						motion_estimate_deg += (rotation_angle_deg - motion_estimate_deg) * motion_smoothing;

						const float rotation_t = std::min(1.0f, motion_estimate_deg / STABILIZER_ANGLE_THRESHOLD_DEG);
						const float stabilizer_tau = phone.stabilizer_tau_max + (STABILIZER_TAU_MIN - phone.stabilizer_tau_max) * rotation_t;

						const float smoothing = 1.0f - std::exp(-dt / stabilizer_tau);
						smoothed_rotation = phone_math::quat_slerp(smoothed_rotation, phone.rotation, smoothing);
					}

					// Extra 90 degree roll
					if (phone.portrait_mode) {
						const prism::quat_t portrait_roll = phone_math::quat_from_axis_angle(0.0, 0.0, 1.0, PI / 2.0);
						freecam->m_placement.rotation = phone_math::quat_multiply(smoothed_rotation, portrait_roll);
					}
					else {
						freecam->m_placement.rotation = smoothed_rotation;
					}

					// Horizontal is relative to look direction, vertical is always world up/down
					prism::float3_t forward = phone_math::rotate_vector(phone.rotation, { 1.0f, 0.0f, 0.0f });
					prism::float3_t right = phone_math::rotate_vector(phone.rotation, { 0.0f, 0.0f, 1.0f });
					forward.y = 0.0f;
					right.y = 0.0f;
					forward = phone_math::normalize(forward);
					right = phone_math::normalize(right);

					float target_vel_x = 0.0f;
					float target_vel_z = 0.0f;
					if (phone.joystick.x != 0.0f || phone.joystick.y != 0.0f)
					{
						const float forward_amount = phone.joystick.x;
						const float right_amount = -phone.joystick.y;

						target_vel_x = (forward.x * forward_amount + right.x * right_amount) * phone.move_speed;
						target_vel_z = (forward.z * forward_amount + right.z * right_amount) * phone.move_speed;

						const float step_pulse = 1.0f + STEP_SPEED_VARIATION * phone.head_bob_strength * std::sin(2.0f * step_phase);
						target_vel_x *= step_pulse;
						target_vel_z *= step_pulse;
					}
					const float target_vel_y = phone.joystick.z * phone.move_speed;

					// Stopping carries more momentum than starting
					const float target_speed_sq = target_vel_x * target_vel_x + target_vel_z * target_vel_z;
					const float current_speed_sq = vel_x * vel_x + vel_z * vel_z;
					const float horizontal_tau = (target_speed_sq > current_speed_sq) ? phone.walk_accel_time : phone.walk_accel_time * DECEL_TAU_MULTIPLIER;

					const float horizontal_smoothing = 1.0f - std::exp(-dt / horizontal_tau);
					const float vertical_smoothing = 1.0f - std::exp(-dt / phone.walk_accel_time);
					vel_x += (target_vel_x - vel_x) * horizontal_smoothing;
					vel_z += (target_vel_z - vel_z) * horizontal_smoothing;
					vel_y += (target_vel_y - vel_y) * vertical_smoothing;

					walk_pos.x += vel_x * dt;
					walk_pos.z += vel_z * dt;
					walk_pos.y += vel_y * dt;

					// Head bob and idle sway crossfade by speed and are applied as a pure offset on top of walk_pos
					const float horizontal_speed = std::sqrt(vel_x * vel_x + vel_z * vel_z);
					step_phase = std::fmod(step_phase + horizontal_speed * dt * (2.0f * PI / STRIDE_LENGTH), 2.0f * PI);
					idle_phase = std::fmod(idle_phase + dt * IDLE_SWAY_SPEED, 2.0f * PI);

					const float walk_fade = std::min(1.0f, horizontal_speed / BOB_FADE_SPEED);
					const float idle_fade = 1.0f - walk_fade;

					const float walk_vertical = HEAD_BOB_HEIGHT * phone.head_bob_strength * walk_fade * std::abs(std::sin(step_phase));
					const float walk_lateral = HEAD_BOB_SWAY * phone.head_bob_strength * walk_fade * std::sin(step_phase);

					const float idle_vertical = IDLE_SWAY_HEIGHT * phone.idle_sway_strength * idle_fade * std::sin(idle_phase);
					const float idle_lateral = IDLE_SWAY_WIDTH * phone.idle_sway_strength * idle_fade * std::sin(idle_phase * 0.7f);

					const float vertical_offset = walk_vertical + idle_vertical;
					const float lateral_offset = walk_lateral + idle_lateral;

					freecam->m_placement.position.x = walk_pos.x + right.x * lateral_offset;
					freecam->m_placement.position.y = walk_pos.y + vertical_offset;
					freecam->m_placement.position.z = walk_pos.z + right.z * lateral_offset;

					// fov is a scale factor, 1.0 = unchanged
					if (phone.fov > 0.0f)
					{
						const float zoom_smoothing = 1.0f - std::exp(-dt / phone.zoom_time_constant);
						smoothed_fov += (phone.fov - smoothed_fov) * zoom_smoothing;

						constexpr float deg2rad = PI / 180.0f;
						constexpr float rad2deg = 180.0f / PI;

						const float base_v_half_tan = std::tan(base_v_fov * 0.5f * deg2rad);
						const float aspect = std::tan(base_h_fov * 0.5f * deg2rad) / base_v_half_tan;

						const float new_v_half_tan = base_v_half_tan * smoothed_fov;
						const float new_h_half_tan = new_v_half_tan * aspect;

						freecam->m_vertical_fov = 2.0f * std::atan(new_v_half_tan) * rad2deg;
						freecam->m_horizontal_fov = 2.0f * std::atan(new_h_half_tan) * rad2deg;
					}
				}
				was_applying_control = phone.control_enabled;
			}
			else
			{
				was_applying_control = false;
			}
		}
		else
		{
			CameraInput::get()->set_free_camera_active(false);
		}
	}
}

#pragma comment( linker, "/export:scs_telemetry_init=scs_telemetry_init" )
SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t* const params)
{
	scs_logging::init(params, "ETS2MobileCam");

	scs_log(0, "ETS2MobileCam | By: Baldy09");
	scs_log(0, "Plugin Loading...");

	freecam_tick_function_address = bmem::patternScan("40 53 48 83 EC ?? 48 83 B9 ?? ?? ?? ?? 00 48 8B D9 0F 29 74 24");
	if (!bmem::isAddressValid(freecam_tick_function_address))
	{
		scs_log(2, "Cannot find freecam_tick_function_address");
		return SCS_RESULT_not_found;
	}

	PhoneServer::get()->start();
	PCInput::get()->start();

	const scs_telemetry_init_params_v101_t* version_params = reinterpret_cast<const scs_telemetry_init_params_v101_t*>(params);
	version_params->register_for_event(SCS_TELEMETRY_EVENT_frame_end, telemetry_tick, nullptr);

	scs_log(0, "Plugin loaded");
	return SCS_RESULT_ok;
}

#pragma comment( linker, "/export:scs_telemetry_shutdown=scs_telemetry_shutdown" )
SCSAPI_VOID scs_telemetry_shutdown()
{
	DWORD old;
	VirtualProtect(reinterpret_cast<void*>(freecam_tick_function_address), 2, PAGE_EXECUTE_READWRITE, &old);
	*reinterpret_cast<uint8_t*>(freecam_tick_function_address + 0) = 0x40;
	VirtualProtect(reinterpret_cast<void*>(freecam_tick_function_address), 2, old, &old);

	PCInput::get()->stop();
	PhoneServer::get()->stop();
	scs_logging::shutdown();
}
