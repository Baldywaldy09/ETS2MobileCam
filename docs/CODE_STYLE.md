# Code Style & Formatting
This document covers formatting and file structure. For identifier naming rules (classes, members, structs, functions), see [`NAMING_CONVENTIONS.md`](NAMING_CONVENTIONS.md).

## File Header
Every `.h`/`.cpp` file begins with this exact block, before anything else, including `#pragma once`:

```cpp
/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: dir/dir/file.h
/*-----------------------------------------*/
```
- `File` is the path relative to `src/`, using forward slashes.
- The colon after `Project` and `File` lines up in the same column, done with tabs (`Project\t:`, `File\t\t:`).

## Indentation
- Indentation uses tabs, not spaces, in both `.h` and `.cpp` files.

## Include Guards
- `#pragma once` is used in every header. `#ifndef`/`#define` guards are not used.

## Const, Pointers & References
- `const` goes before the type: `const std::string&`, not `std::string const&`.
- `*` and `&` stick to the type, not the variable name: `T* ptr`, `const T& x`, not `T *ptr` or `const T &x`.
- Methods that don't change the object are marked `const`. `[[nodiscard]]` is added to getters and any other call whose return value should not be ignored.

```cpp
[[nodiscard]] bool active() const { return m_active; }
void set_device_rotation(const prism::quat_t& rotation);
```

## Class Layout
- `public:` comes first (constructor/destructor, `get()`, public API), then `private:` with `s_instance` and all `m_*` members. Don't mix public and private sections together.
- See [`NAMING_CONVENTIONS.md`](NAMING_CONVENTIONS.md) for the rule on member visibility (members are always private).

## Namespaces
- There is no single project-wide namespace. The singleton classes (`PhoneServer`, `CameraInput`, etc.) live in the global namespace.
- `prism::` holds the reverse-engineered engine types and patterns, under `Prism3D/`.
- `phone_math::` holds the orientation/quaternion math helpers (`Utils/orientation_math.h`).
- `scs_logging::` wraps the SCS SDK's logging callback.
- `base64::`, `sha1::` and `bmem::` are small, self-contained helpers (`Utils/`, `bmem.h`).
- An anonymous namespace is used for helpers that only matter inside one file and shouldn't be visible anywhere else (e.g. `key_down` in `PCInput.cpp` and `find_game_window` in `ScreenCapture.cpp`).

## File Organization
- One class per file pair, file named after the class - `PhoneServer.h`/`PhoneServer.cpp`, `ScreenCapture.h`/`ScreenCapture.cpp`.
- `Net/` - the phone-facing side: the HTTP/websocket server, screen capture/casting, shared camera input state, and the embedded phone control page.
- `Utils/` - small, self-contained helpers (orientation math, base64, SHA-1) that don't depend on the rest of the project.
- `Prism3D/` - reverse-engineered engine types, offsets and other game engine data.
