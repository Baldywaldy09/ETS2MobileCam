/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: PCInput.h
/*-----------------------------------------*/

#pragma once
#include <windows.h>
#include <mutex>
#include <thread>

#include "Prism3D/Containers/math.h"

class PCInput
{
public:
	static PCInput* get();

	void start();
	void stop();

	// Returns true on the exact frame walk mode was just toggled on/off
	bool poll_toggle();

	[[nodiscard]] bool active() const {
		return m_active;
	}

	void deactivate() {
		set_active(false);
	}

	// WASD (Space/Ctrl for vertical) as a joystick-style vector
	[[nodiscard]] prism::float3_t poll_movement() const;

	// Raw mouse delta built up since the last call, zero if inactive or unfocused
	void poll_mouse_delta(float& dx, float& dy);

	// Wheel notches scrolled since the last call (positive = zoom in)
	[[nodiscard]] float poll_wheel_delta();

	// True on the exact call the middle mouse button was pressed since the last call
	[[nodiscard]] bool poll_middle_click();

private:
	PCInput() = default;
	PCInput(const PCInput&) = delete;
	PCInput& operator=(const PCInput&) = delete;

	void set_active(bool active);
	[[nodiscard]] bool game_focused() const;
	void register_raw_input();
	void unregister_raw_input();

	void pump_thread_main();
	static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void handle_raw_input(HRAWINPUT handle);

	bool m_active{ false };
	bool m_toggle_was_down{ false };

	std::thread m_pump_thread;
	HWND m_message_hwnd{};
	DWORD m_pump_thread_id{};

	std::mutex m_input_mutex;
	float m_accum_dx{ 0.0f };
	float m_accum_dy{ 0.0f };
	float m_accum_wheel{ 0.0f };
	bool m_middle_clicked{ false };
};
