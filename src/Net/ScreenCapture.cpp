/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Net/ScreenCapture.cpp
/*-----------------------------------------*/

#include "ScreenCapture.h"

#include <algorithm>
#include <chrono>
#include <vector>
#include <windows.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "CameraInput.h"
#include "scs_logging.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
	// Largest visible top level window owned by this process
	HWND find_game_window()
	{
		struct EnumState
		{
			DWORD pid;
			HWND best{};
			long best_area{};
		} state{ GetCurrentProcessId() };

		EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL
		{
			auto* s = reinterpret_cast<EnumState*>(lparam);

			DWORD owner_pid = 0;
			GetWindowThreadProcessId(hwnd, &owner_pid);
			if (owner_pid != s->pid || !IsWindowVisible(hwnd)) return TRUE;

			RECT rect{};
			if (!GetClientRect(hwnd, &rect)) return TRUE;

			const long area = (rect.right - rect.left) * (rect.bottom - rect.top);
			if (area > s->best_area)
			{
				s->best_area = area;
				s->best = hwnd;
			}
			return TRUE;
		}, reinterpret_cast<LPARAM>(&state));

		return state.best;
	}

	void write_to_vector(void* context, void* data, int size)
	{
		auto* out = reinterpret_cast<std::vector<uint8_t>*>(context);
		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
		out->insert(out->end(), bytes, bytes + size);
	}
}

ScreenCapture* ScreenCapture::get()
{
	static ScreenCapture instance;
	return &instance;
}

void ScreenCapture::start(FrameCallback callback)
{
	if (m_running) return;
	m_running = true;
	m_callback = std::move(callback);
	m_thread = std::thread([this] { capture_loop(); });
}

void ScreenCapture::stop()
{
	if (!m_running) return;
	m_running = false;
	if (m_thread.joinable()) m_thread.join();
}

bool ScreenCapture::init_duplication()
{
	D3D_FEATURE_LEVEL feature_level{};
	HRESULT hr = D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
		nullptr, 0, D3D11_SDK_VERSION,
		&m_device, &feature_level, &m_context);
	if (FAILED(hr))
	{
		scs_logging::scs_log(1, "Screen capture: D3D11CreateDevice failed (0x%08lX)", static_cast<unsigned long>(hr));
		return false;
	}

	IDXGIDevice* dxgi_device = nullptr;
	m_device->QueryInterface(IID_PPV_ARGS(&dxgi_device));

	IDXGIAdapter* adapter = nullptr;
	dxgi_device->GetAdapter(&adapter);
	dxgi_device->Release();

	IDXGIOutput* output = nullptr;
	hr = adapter->EnumOutputs(0, &output);
	adapter->Release();
	if (FAILED(hr))
	{
		scs_logging::scs_log(1, "Screen capture: EnumOutputs failed (0x%08lX)", static_cast<unsigned long>(hr));
		return false;
	}

	IDXGIOutput1* output1 = nullptr;
	output->QueryInterface(IID_PPV_ARGS(&output1));
	output->Release();

	hr = output1->DuplicateOutput(m_device, &m_duplication);
	output1->Release();
	if (FAILED(hr))
	{
		scs_logging::scs_log(1, "Screen capture: DuplicateOutput failed (0x%08lX)", static_cast<unsigned long>(hr));
		return false;
	}

	return true;
}

void ScreenCapture::shutdown_duplication()
{
	if (m_staging_texture) { m_staging_texture->Release(); m_staging_texture = nullptr; }
	if (m_duplication) { m_duplication->Release(); m_duplication = nullptr; }
	if (m_context) { m_context->Release(); m_context = nullptr; }
	if (m_device) { m_device->Release(); m_device = nullptr; }
	m_staging_width = m_staging_height = 0;
}

void ScreenCapture::capture_loop()
{
	if (!init_duplication())
	{
		m_running = false;
		return;
	}

	std::vector<uint8_t> rgb_buffer;
	std::vector<uint8_t> rotated_buffer;
	std::vector<uint8_t> jpeg_buffer;

	while (m_running)
	{
		const auto frame_start = std::chrono::steady_clock::now();

		IDXGIResource* resource = nullptr;
		DXGI_OUTDUPL_FRAME_INFO frame_info{};
		HRESULT hr = m_duplication->AcquireNextFrame(100, &frame_info, &resource);

		if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		{
			continue;
		}
		if (FAILED(hr))
		{
			shutdown_duplication();
			if (!init_duplication()) break;
			continue;
		}

		ID3D11Texture2D* frame_texture = nullptr;
		resource->QueryInterface(IID_PPV_ARGS(&frame_texture));
		resource->Release();

		D3D11_TEXTURE2D_DESC desc{};
		frame_texture->GetDesc(&desc);

		if (!m_staging_texture || m_staging_width != desc.Width || m_staging_height != desc.Height)
		{
			if (m_staging_texture) { m_staging_texture->Release(); m_staging_texture = nullptr; }

			D3D11_TEXTURE2D_DESC staging_desc = desc;
			staging_desc.Usage = D3D11_USAGE_STAGING;
			staging_desc.BindFlags = 0;
			staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			staging_desc.MiscFlags = 0;

			if (SUCCEEDED(m_device->CreateTexture2D(&staging_desc, nullptr, &m_staging_texture)))
			{
				m_staging_width = desc.Width;
				m_staging_height = desc.Height;
			}
		}

		// Read fresh each frame so a settings change takes effect immediately
		const camera_input_state_t phone = CameraInput::get()->get_state();
		const int max_dimension = static_cast<int>(std::clamp(phone.cast_resolution, 240.0f, 1920.0f));
		const int jpeg_quality = static_cast<int>(std::clamp(phone.cast_quality, 10.0f, 100.0f));

		if (m_staging_texture)
		{
			m_context->CopyResource(m_staging_texture, frame_texture);

			D3D11_MAPPED_SUBRESOURCE mapped{};
			if (SUCCEEDED(m_context->Map(m_staging_texture, 0, D3D11_MAP_READ, 0, &mapped)))
			{
				// Crop to the games client rect, otherwise the whole monitor
				RECT crop{ 0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height) };
				if (const HWND game_window = find_game_window())
				{
					RECT client_rect{};
					POINT origin{ 0, 0 };
					if (GetClientRect(game_window, &client_rect) && ClientToScreen(game_window, &origin))
					{
						crop.left = std::max<LONG>(0, origin.x);
						crop.top = std::max<LONG>(0, origin.y);
						crop.right = std::min<LONG>(static_cast<LONG>(desc.Width), origin.x + client_rect.right);
						crop.bottom = std::min<LONG>(static_cast<LONG>(desc.Height), origin.y + client_rect.bottom);
					}
				}

				const int crop_w = static_cast<int>(crop.right - crop.left);
				const int crop_h = static_cast<int>(crop.bottom - crop.top);

				if (crop_w > 0 && crop_h > 0)
				{
					const float scale = std::min(1.0f, static_cast<float>(max_dimension) / static_cast<float>(std::max(crop_w, crop_h)));
					const int scaled_w = std::max(1, static_cast<int>(crop_w * scale));
					const int scaled_h = std::max(1, static_cast<int>(crop_h * scale));

					rgb_buffer.resize(static_cast<size_t>(scaled_w) * scaled_h * 3);
					const uint8_t* src_base = reinterpret_cast<const uint8_t*>(mapped.pData);

					for (int y = 0; y < scaled_h; ++y)
					{
						const int src_y = crop.top + (y * crop_h) / scaled_h;
						const uint8_t* src_row = src_base + static_cast<size_t>(src_y) * mapped.RowPitch;
						uint8_t* dst_row = rgb_buffer.data() + static_cast<size_t>(y) * scaled_w * 3;

						for (int x = 0; x < scaled_w; ++x)
						{
							const int src_x = crop.left + (x * crop_w) / scaled_w;
							const uint8_t* src_pixel = src_row + static_cast<size_t>(src_x) * 4;

							// DXGI gives BGRA, stb_image_write wants RGB
							dst_row[x * 3 + 0] = src_pixel[2];
							dst_row[x * 3 + 1] = src_pixel[1];
							dst_row[x * 3 + 2] = src_pixel[0];
						}
					}

					// Counter rotate so the phone sees an upright frame in portrait mode
					int out_w = scaled_w;
					int out_h = scaled_h;
					const uint8_t* encode_source = rgb_buffer.data();

					if (phone.portrait_mode)
					{
						out_w = scaled_h;
						out_h = scaled_w;
						rotated_buffer.resize(static_cast<size_t>(out_w) * out_h * 3);

						for (int y = 0; y < scaled_h; ++y)
						{
							for (int x = 0; x < scaled_w; ++x)
							{
								// 90 degree counter-clockwise
								const int dst_x = y;
								const int dst_y = scaled_w - 1 - x;
								const uint8_t* src_pixel = rgb_buffer.data() + (static_cast<size_t>(y) * scaled_w + x) * 3;
								uint8_t* dst_pixel = rotated_buffer.data() + (static_cast<size_t>(dst_y) * out_w + dst_x) * 3;
								dst_pixel[0] = src_pixel[0];
								dst_pixel[1] = src_pixel[1];
								dst_pixel[2] = src_pixel[2];
							}
						}

						encode_source = rotated_buffer.data();
					}

					jpeg_buffer.clear();
					stbi_write_jpg_to_func(write_to_vector, &jpeg_buffer, out_w, out_h, 3, encode_source, jpeg_quality);

					if (m_callback && !jpeg_buffer.empty()) m_callback(jpeg_buffer.data(), jpeg_buffer.size());
				}

				m_context->Unmap(m_staging_texture, 0);
			}
		}

		frame_texture->Release();
		m_duplication->ReleaseFrame();

		const float fps = std::clamp(phone.cast_fps, 1.0f, 30.0f);
		const auto frame_interval = std::chrono::milliseconds(static_cast<int>(1000.0f / fps));

		const auto elapsed = std::chrono::steady_clock::now() - frame_start;
		if (elapsed < frame_interval) std::this_thread::sleep_for(frame_interval - elapsed);
	}

	shutdown_duplication();
}
