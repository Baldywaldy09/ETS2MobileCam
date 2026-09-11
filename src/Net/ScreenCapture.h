/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Net/ScreenCapture.h
/*-----------------------------------------*/

#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>

#include <d3d11.h>
#include <dxgi1_2.h>

// Captures the games window via DXGI Desktop Duplication, jpeg encodes each frame and hands it to a callback
class ScreenCapture
{
public:
	using FrameCallback = std::function<void(const uint8_t* jpeg_data, size_t jpeg_size)>;

	static ScreenCapture* get();

	void start(FrameCallback callback);
	void stop();

private:
	ScreenCapture() = default;
	ScreenCapture(const ScreenCapture&) = delete;
	ScreenCapture& operator=(const ScreenCapture&) = delete;

	void capture_loop();
	bool init_duplication();
	void shutdown_duplication();

	std::atomic<bool> m_running{ false };
	std::thread m_thread;
	FrameCallback m_callback;

	ID3D11Device* m_device{};
	ID3D11DeviceContext* m_context{};
	IDXGIOutputDuplication* m_duplication{};
	ID3D11Texture2D* m_staging_texture{};
	UINT m_staging_width{};
	UINT m_staging_height{};
};
