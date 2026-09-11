/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: PCInput.cpp
/*-----------------------------------------*/

#include "PCInput.h"

namespace
{
	constexpr int TOGGLE_KEY = VK_F9;
	constexpr wchar_t WINDOW_CLASS_NAME[] = L"ETS2MobileCamPCInputWnd";

	[[nodiscard]] bool key_down(int vk)
	{
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}
}

PCInput* PCInput::get()
{
	static PCInput instance;
	return &instance;
}

LRESULT CALLBACK PCInput::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_INPUT)
	{
		PCInput::get()->handle_raw_input(reinterpret_cast<HRAWINPUT>(lparam));
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void PCInput::handle_raw_input(HRAWINPUT handle)
{
	UINT size = 0;
	GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
	if (size == 0 || size > 1024) return;

	BYTE buffer[1024];
	if (GetRawInputData(handle, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) != size) return;

	const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer);
	if (raw->header.dwType != RIM_TYPEMOUSE) return;
	const RAWMOUSE& mouse = raw->data.mouse;

	std::lock_guard<std::mutex> lock(m_input_mutex);

	if (!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) // ignore absolute-position devices
	{
		m_accum_dx += static_cast<float>(mouse.lLastX);
		m_accum_dy += static_cast<float>(mouse.lLastY);
	}

	if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
		m_accum_wheel += static_cast<SHORT>(mouse.usButtonData) / static_cast<float>(WHEEL_DELTA);

	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
		m_middle_clicked = true;
}

void PCInput::register_raw_input()
{
	if (!m_message_hwnd) return;

	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01; // generic desktop
	rid.usUsage = 0x02;     // mouse
	rid.dwFlags = RIDEV_INPUTSINK; // keep receiving input even without focus
	rid.hwndTarget = m_message_hwnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void PCInput::unregister_raw_input()
{
	// register only while walk mode is active, otherwise it steals the games own target
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = RIDEV_REMOVE;
	rid.hwndTarget = nullptr;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void PCInput::pump_thread_main()
{
	m_pump_thread_id = GetCurrentThreadId();

	WNDCLASSW wc{};
	wc.lpfnWndProc = &PCInput::wnd_proc;
	wc.hInstance = GetModuleHandleW(nullptr);
	wc.lpszClassName = WINDOW_CLASS_NAME;
	RegisterClassW(&wc);

	m_message_hwnd = CreateWindowExW(0, WINDOW_CLASS_NAME, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
	if (m_message_hwnd)
	{
		// Raw input is only registered while walk mode is active, see set_active
		MSG msg{};
		while (GetMessageW(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		DestroyWindow(m_message_hwnd);
		m_message_hwnd = nullptr;
	}

	UnregisterClassW(WINDOW_CLASS_NAME, wc.hInstance);
}

void PCInput::start()
{
	if (m_pump_thread.joinable()) return;
	m_pump_thread = std::thread([this] { pump_thread_main(); });
}

void PCInput::stop()
{
	if (m_active) unregister_raw_input();
	if (!m_pump_thread.joinable()) return;
	if (m_pump_thread_id) PostThreadMessageW(m_pump_thread_id, WM_QUIT, 0, 0);
	m_pump_thread.join();
}

bool PCInput::poll_toggle()
{
	// Auto disable on tab out
	if (m_active && !game_focused()) {
		set_active(false);
	}

	const bool down = key_down(TOGGLE_KEY);
	const bool pressed = down && !m_toggle_was_down;
	m_toggle_was_down = down;

	if (pressed) set_active(!m_active);
	return pressed;
}

void PCInput::set_active(bool active)
{
	m_active = active;
	ShowCursor(active ? FALSE : TRUE);

	if (active) {
		register_raw_input();
	}
	else {
		unregister_raw_input();
	}
}

bool PCInput::game_focused() const
{
	const HWND fg = GetForegroundWindow();
	if (!fg) return false;

	DWORD pid = 0;
	GetWindowThreadProcessId(fg, &pid);
	return pid == GetCurrentProcessId();
}

prism::float3_t PCInput::poll_movement() const
{
	if (!m_active || !game_focused()) return { 0.0f, 0.0f, 0.0f };

	const float forward_amount = (key_down('W') ? 1.0f : 0.0f) - (key_down('S') ? 1.0f : 0.0f);
	const float right_amount = (key_down('D') ? 1.0f : 0.0f) - (key_down('A') ? 1.0f : 0.0f);
	const float vertical = (key_down(VK_SPACE) ? 1.0f : 0.0f) - (key_down(VK_CONTROL) ? 1.0f : 0.0f);

	return { right_amount, forward_amount, vertical };
}

void PCInput::poll_mouse_delta(float& dx, float& dy)
{
	dx = 0.0f;
	dy = 0.0f;

	std::lock_guard<std::mutex> lock(m_input_mutex);
	if (m_active && game_focused())
	{
		dx = m_accum_dx;
		dy = m_accum_dy;
	}
	m_accum_dx = 0.0f;
	m_accum_dy = 0.0f;
}

float PCInput::poll_wheel_delta()
{
	std::lock_guard<std::mutex> lock(m_input_mutex);
	const float value = (m_active && game_focused()) ? m_accum_wheel : 0.0f;
	m_accum_wheel = 0.0f;
	return value;
}

bool PCInput::poll_middle_click()
{
	std::lock_guard<std::mutex> lock(m_input_mutex);
	const bool clicked = m_middle_clicked;
	m_middle_clicked = false;
	return clicked && m_active && game_focused();
}
