/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: globals.h
/*-----------------------------------------*/

#pragma once

inline constexpr float PI = 3.14159265358979323846f;

// Adaptive rotation stabilizer bounds
inline constexpr float STABILIZER_TAU_MIN = 0.03f;
inline constexpr float STABILIZER_ANGLE_THRESHOLD_DEG = 12.0f;

// Websocket handshake GUID (RFC 6455)
inline constexpr char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Walk cycle | footstep driven movement feel
inline constexpr float STRIDE_LENGTH = 1.4f;			// engine units per full left-right gait cycle
inline constexpr float HEAD_BOB_HEIGHT = 0.035f;		// vertical bob amplitude at full speed
inline constexpr float HEAD_BOB_SWAY = 0.02f;			// lateral sway amplitude at full speed
inline constexpr float BOB_FADE_SPEED = 0.4f;			// units/sec at which bob reaches full amplitude
inline constexpr float STEP_SPEED_VARIATION = 0.06f;	// fractional speed pulse per footfall
inline constexpr float DECEL_TAU_MULTIPLIER = 1.6f;	    // stopping takes longer than starting

// Idle sway | drift while standing still, crossfades with the walk bob above
inline constexpr float IDLE_SWAY_HEIGHT = 0.01f;
inline constexpr float IDLE_SWAY_WIDTH  = 0.012f;
inline constexpr float IDLE_SWAY_SPEED  = 0.6f; // radians/sec

// PC walk mode | PCInput.h
inline constexpr float PC_MOUSE_SENSITIVITY = 0.0018f;	// radians per pixel of mouse delta - tune to taste
inline constexpr float PC_PITCH_LIMIT_DEG = 89.0f;		// clamp look up/down short of straight up/down

// Natural handheld lean into turns, proportional to turn rate (not amount) so it settles back to level once the mouse stops. Scaled down while standing still, full while walking.
inline constexpr float PC_TILT_STRENGTH = 0.08f;		    // tilt radians per (yaw radians/sec)
inline constexpr float PC_TILT_MAX_DEG = 3.0f;			    // clamp
inline constexpr float PC_TILT_TAU = 0.25f;			        // seconds, eases the tilt in/out
inline constexpr float PC_TILT_STATIONARY_FRACTION = 0.2f;  // tilt strength fraction while standing still

// Scroll wheel zoom - same fov scale-factor units the phone's slider uses (1.0 = unchanged)
inline constexpr float PC_ZOOM_STEP = 0.1f; // fov scale change per wheel notch
inline constexpr float PC_ZOOM_MIN  = 0.1f; // matches the phone's slider range (10%)
inline constexpr float PC_ZOOM_MAX  = 4.0f; // matches the phone's slider range (400%)
