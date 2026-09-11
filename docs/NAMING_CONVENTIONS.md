# Naming Conventions
This document defines the naming rules for ETS2MobileCam C++ code. Code that doesn't follow these rules should be treated as legacy: new code must follow this guide, and old code should be updated when possible, rather than copied as an example.

## Classes
- Class names are **PascalCase**: `PhoneServer`, `CameraInput`, `PCInput`, `ScreenCapture`.
- The file name matches the class name exactly: `PhoneServer` lives in `PhoneServer.h` / `PhoneServer.cpp`.
- A class's constructor, `get()`, and public API follow the same rules as any other method below - nothing in this project forces PascalCase on them.

## Member Variables
- Member names are **snake_case**.
- Instance members start with `m_`: `m_active`, `m_running`, `m_ws_sockets`.
- Static members start with `s_` where used.
- **Members are always private.** Every field sits under a `private:` section and is only read or changed through the class's own methods. If code outside the class needs a value, add an accessor method for it instead of making the field public.
- Accessors are just normal methods (see [Functions & Local Variables](#functions--local-variables)) - there's no special naming rule for getters and setters, `get_x()` / `set_x()` follow the same snake_case rule as any other function.

```cpp
class PCInput
{
public:
	static PCInput* get();
	[[nodiscard]] bool active() const { return m_active; }

private:
	bool m_active{ false };
	std::thread m_pump_thread;
};
```

## Structs & Enums
- Struct names are **snake_case**.
- Add a `_t` suffix when the plain name could be read as a value, or would be unclear without it, e.g. `float3_t`, `quat_t`, `matrix4x4_t`, `camera_input_state_t`, `parsed_request_t`.
- This applies to plain-data structs in general: wire/message structs, engine-mirrored Prism3D types, and local helper structs. It does not apply to types from external/vendored libraries (`nlohmann::json`, `asio::*`).
- The project doesn't currently have any `enum class` types. If one is added, use a scoped `enum class` with a snake_case name and `ALL_CAPS` enumerators.

```cpp
struct camera_input_state_t
{
	prism::quat_t rotation{ 1.f, 0.f, 0.f, 0.f };
	prism::float3_t joystick{ 0.f, 0.f, 0.f };
	float fov{ -1.f };
};
```

## Functions & Local Variables
- Regular methods and free functions are **snake_case**: `poll_toggle()`, `set_device_rotation()`, `handle_ws_connection()`.
- Win32 callback functions (e.g. `PCInput::wnd_proc`) still use snake_case for the name itself; only the calling convention (`CALLBACK`) and parameter types come from the API.
- Local variables are snake_case: `pc_yaw`, `mouse_dx`, `freecam_tick_patched`.

## Constants
- Project-wide tunables live in `globals.h` as `inline constexpr`, using `SCREAMING_SNAKE_CASE`.
- File-local constants use the same `SCREAMING_SNAKE_CASE` style inside an anonymous namespace, e.g. `TOGGLE_KEY`, `WINDOW_CLASS_NAME` in `PCInput.cpp`.

## Cases Not to Use
- **camelCase** is not used anywhere in this codebase: not for members (`m_someValue`), functions (`updateCamera()`), or local variables (`posX`).

## Quick Reference

| Kind                          | Style                          | Example                    |
|-------------------------------|---------------------------------|-----------------------------|
| Class + file                  | PascalCase                     | `PhoneServer`               |
| Member variable (instance)    | `m_` + snake_case              | `m_ws_sockets`               |
| Struct                        | snake_case (`_t` when needed)  | `camera_input_state_t`      |
| Function / method             | snake_case                     | `poll_mouse_delta()`        |
| Local variable                | snake_case                     | `pc_yaw`                    |
| Constant (`globals.h` / file-local) | `SCREAMING_SNAKE_CASE` | `PC_MOUSE_SENSITIVITY`      |
