/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Net/PhoneServer.cpp
/*-----------------------------------------*/

#include "PhoneServer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <istream>

#include <nlohmann/json.hpp>

#include "CameraInput.h"
#include "ScreenCapture.h"
#include "globals.h"
#include "phone_page.h"
#include "scs_logging.h"
#include "Utils/orientation_math.h"
#include "Utils/sha1.h"
#include "Utils/base64.h"

using json = nlohmann::json;

namespace
{
	[[nodiscard]] std::string trim(const std::string& s)
	{
		const size_t start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) return "";
		const size_t end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	}

	[[nodiscard]] std::string to_lower(std::string s)
	{
		for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return s;
	}

	[[nodiscard]] bool contains_ci(const std::string& haystack, const std::string& needle)
	{
		return to_lower(haystack).find(to_lower(needle)) != std::string::npos;
	}

	// Server to client frames are never masked
	bool write_ws_binary_frame(asio::ip::tcp::socket& socket, const uint8_t* data, size_t length)
	{
		std::string header;
		header.push_back(static_cast<char>(0x82)); // FIN + binary opcode

		if (length <= 125)
		{
			header.push_back(static_cast<char>(length));
		}
		else if (length <= 0xFFFF)
		{
			header.push_back(static_cast<char>(126));
			header.push_back(static_cast<char>((length >> 8) & 0xFF));
			header.push_back(static_cast<char>(length & 0xFF));
		}
		else
		{
			header.push_back(static_cast<char>(127));
			for (int shift = 56; shift >= 0; shift -= 8)
				header.push_back(static_cast<char>((static_cast<uint64_t>(length) >> shift) & 0xFF));
		}

		asio::error_code ec;
		asio::write(socket, asio::buffer(header), ec);
		if (ec) return false;
		asio::write(socket, asio::buffer(data, length), ec);
		return !ec;
	}
}

PhoneServer* PhoneServer::get()
{
	static PhoneServer instance;
	return &instance;
}

void PhoneServer::start(uint16_t port)
{
	if (m_running) return;
	m_running = true;

	m_io_context = std::make_unique<asio::io_context>();

	m_acceptor = std::make_unique<asio::ip::tcp::acceptor>(
		*m_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

	m_accept_thread = std::thread([this] { run_accept_loop(); });

	scs_logging::scs_log(0, "Phone control page: http://<this-pc-lan-ip>:%d/", static_cast<int>(port));
}

void PhoneServer::stop()
{
	if (!m_running) return;
	m_running = false;

	m_casting_enabled = false;
	ScreenCapture::get()->stop();
	m_latest_frame_cv.notify_all();
	if (m_cast_sender_thread.joinable()) m_cast_sender_thread.join();

	asio::error_code ec;
	if (m_acceptor) m_acceptor->close(ec);

	{
		std::lock_guard<std::mutex> lock(m_ws_sockets_mutex);
		for (auto& socket : m_ws_sockets)
			socket->close(ec);
		m_ws_sockets.clear();
	}

	if (m_accept_thread.joinable()) m_accept_thread.join();

	join_workers();

	m_acceptor.reset();
	m_io_context.reset();

	CameraInput::get()->set_connected(false);
}

void PhoneServer::track_worker(std::thread thread)
{
	std::lock_guard<std::mutex> lock(m_workers_mutex);
	m_workers.push_back(std::move(thread));
}

void PhoneServer::join_workers()
{
	std::lock_guard<std::mutex> lock(m_workers_mutex);
	for (auto& worker : m_workers)
		if (worker.joinable()) worker.join();
	m_workers.clear();
}

void PhoneServer::run_accept_loop()
{
	while (m_running)
	{
		auto socket = std::make_shared<asio::ip::tcp::socket>(*m_io_context);
		asio::error_code ec;
		m_acceptor->accept(*socket, ec);
		if (ec || !m_running) continue;

		track_worker(std::thread([this, socket] { handle_connection(socket); }));
	}
}

void PhoneServer::handle_connection(const std::shared_ptr<asio::ip::tcp::socket>& socket)
{
	asio::error_code ec;
	asio::streambuf buffer;
	asio::read_until(*socket, buffer, "\r\n\r\n", ec);
	if (ec) return;

	std::istream stream(&buffer);
	parsed_request_t request;
	std::getline(stream, request.request_line);
	if (!request.request_line.empty() && request.request_line.back() == '\r') request.request_line.pop_back();

	std::string line;
	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty()) break;

		const size_t colon = line.find(':');
		if (colon == std::string::npos) continue;

		request.headers[to_lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
	}

	const auto upgrade_it = request.headers.find("upgrade");
	if (upgrade_it != request.headers.end() && contains_ci(upgrade_it->second, "websocket"))
		handle_ws_connection(socket, request);
	else
		handle_http_request(socket, request);
}

void PhoneServer::handle_http_request(const std::shared_ptr<asio::ip::tcp::socket>& socket, const parsed_request_t& request)
{
	std::string response;

	// Polled by the phone page to check whether the in game camera is the free camera
	if (request.request_line.rfind("GET /status", 0) == 0)
	{
		const camera_input_state_t phone = CameraInput::get()->get_state();
		const json status{ { "free_camera_active", phone.free_camera_active } };
		const std::string body = status.dump();

		response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: " + std::to_string(body.size()) + "\r\n"
			"Connection: close\r\n\r\n" + body;
	}
	else
	{
		response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html; charset=utf-8\r\n"
			"Content-Length: " + std::to_string(phone_page::html.size()) + "\r\n"
			"Connection: close\r\n\r\n" + phone_page::html;
	}

	asio::error_code ec;
	asio::write(*socket, asio::buffer(response), ec);
	socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
}

bool PhoneServer::perform_ws_handshake(asio::ip::tcp::socket& socket, const std::string& ws_key)
{
	const auto digest = sha1::digest(ws_key + WS_GUID);
	const std::string accept_key = base64::encode(digest.data(), digest.size());

	const std::string response =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: " + accept_key + "\r\n\r\n";

	asio::error_code ec;
	asio::write(socket, asio::buffer(response), ec);
	return !ec;
}

void PhoneServer::handle_ws_connection(const std::shared_ptr<asio::ip::tcp::socket>& socket, const parsed_request_t& request)
{
	const auto key_it = request.headers.find("sec-websocket-key");
	if (key_it == request.headers.end() || !perform_ws_handshake(*socket, key_it->second))
	{
		asio::error_code ec;
		socket->close(ec);
		return;
	}

	// Only one phone connection at a time - drop whatever was connected before
	{
		std::lock_guard<std::mutex> lock(m_ws_sockets_mutex);
		for (auto& old_socket : m_ws_sockets)
		{
			asio::error_code close_ec;
			old_socket->close(close_ec);
		}
		m_ws_sockets.clear();
		m_ws_sockets.push_back(socket);
	}

	CameraInput::get()->set_connected(true);
	scs_logging::scs_log(0, "Phone connected");

	while (m_running)
	{
		asio::error_code ec;

		uint8_t header[2]{};
		asio::read(*socket, asio::buffer(header, 2), ec);
		if (ec) break;

		const bool masked = (header[1] & 0x80) != 0;
		uint64_t payload_length = header[1] & 0x7F;
		const uint8_t opcode = header[0] & 0x0F;

		if (payload_length == 126)
		{
			uint8_t ext[2]{};
			asio::read(*socket, asio::buffer(ext, 2), ec);
			if (ec) break;
			payload_length = (static_cast<uint64_t>(ext[0]) << 8) | ext[1];
		}
		else if (payload_length == 127)
		{
			uint8_t ext[8]{};
			asio::read(*socket, asio::buffer(ext, 8), ec);
			if (ec) break;
			payload_length = 0;
			for (unsigned char i : ext)
				payload_length = (payload_length << 8) | i;
		}

		// Sanity cap
		if (payload_length > 8192) break;

		uint8_t mask_key[4]{};
		if (masked)
		{
			asio::read(*socket, asio::buffer(mask_key, 4), ec);
			if (ec) break;
		}

		std::string payload(payload_length, '\0');
		if (payload_length > 0)
		{
			asio::read(*socket, asio::buffer(&payload[0], payload_length), ec);
			if (ec) break;

			if (masked)
				for (uint64_t i = 0; i < payload_length; ++i)
					payload[i] = static_cast<char>(static_cast<uint8_t>(payload[i]) ^ mask_key[i % 4]);
		}

		if (opcode == 0x8) break; // close frame
		if (opcode == 0x1) handle_message(payload); // text frame
	}

	asio::error_code ec;
	socket->close(ec);

	bool still_connected = false;
	{
		std::lock_guard<std::mutex> lock(m_ws_sockets_mutex);
		m_ws_sockets.erase(std::remove(m_ws_sockets.begin(), m_ws_sockets.end(), socket), m_ws_sockets.end());
		still_connected = !m_ws_sockets.empty();
	}

	if (!still_connected)
	{
		CameraInput::get()->set_connected(false);
		scs_logging::scs_log(0, "Phone disconnected");
	}
}

void PhoneServer::handle_message(const std::string& payload)
{
	json parsed;
	try
	{
		parsed = json::parse(payload);
	}
	catch (const json::parse_error&)
	{
		return;
	}

	const std::string type = parsed.value("type", "");

	if (type == "config")
	{
		CameraInput::get()->set_control_enabled(parsed.value("control_enabled", false));
	}
	else if (type == "quat")
	{
		prism::quat_t device_relative{};
		device_relative.w = parsed.value("w", 1.0f);
		device_relative.x = parsed.value("x", 0.0f);
		device_relative.y = parsed.value("y", 0.0f);
		device_relative.z = parsed.value("z", 0.0f);

		CameraInput::get()->set_device_rotation(phone_math::device_quat_to_engine(device_relative));
	}
	else if (type == "trim")
	{
		const double deg2rad = 3.14159265358979323846 / 180.0;
		const double yaw = parsed.value("yaw", 0.0) * deg2rad;
		const double pitch = parsed.value("pitch", 0.0) * deg2rad;
		const double roll = parsed.value("roll", 0.0) * deg2rad;

		const prism::quat_t q_yaw = phone_math::quat_from_axis_angle(0.0, 1.0, 0.0, yaw);
		const prism::quat_t q_pitch = phone_math::quat_from_axis_angle(1.0, 0.0, 0.0, pitch);
		const prism::quat_t q_roll = phone_math::quat_from_axis_angle(0.0, 0.0, 1.0, roll);

		CameraInput::get()->set_trim(phone_math::quat_multiply(phone_math::quat_multiply(q_yaw, q_pitch), q_roll));
	}
	else if (type == "joystick")
	{
		prism::float3_t joystick{};
		joystick.x = parsed.value("x", 0.0f);
		joystick.y = parsed.value("y", 0.0f);
		joystick.z = parsed.value("vertical", 0.0f);

		CameraInput::get()->set_joystick(joystick);
	}
	else if (type == "fov")
	{
		CameraInput::get()->set_fov(parsed.value("value", -1.0f));
	}
	else if (type == "settings")
	{
		CameraInput::get()->set_settings(
			parsed.value("move_speed", 2.5f),
			parsed.value("walk_accel_time", 0.2f),
			parsed.value("zoom_time_constant", 0.18f),
			parsed.value("stabilizer_tau_max", 0.18f),
			parsed.value("cast_fps", 12.0f),
			parsed.value("head_bob_strength", 1.0f),
			parsed.value("cast_quality", 70.0f),
			parsed.value("cast_resolution", 720.0f),
			parsed.value("idle_sway_strength", 1.0f));
	}
	else if (type == "portrait")
	{
		CameraInput::get()->set_portrait_mode(parsed.value("enabled", false));
	}
	else if (type == "cast")
	{
		const bool enabled = parsed.value("enabled", false);

		if (enabled && !m_casting_enabled)
		{
			m_casting_enabled = true;
			ScreenCapture::get()->start([this](const uint8_t* data, size_t size) { on_captured_frame(data, size); });
			m_cast_sender_thread = std::thread([this] { cast_sender_loop(); });
		}
		else if (!enabled && m_casting_enabled)
		{
			m_casting_enabled = false;
			ScreenCapture::get()->stop();
			m_latest_frame_cv.notify_all();
			if (m_cast_sender_thread.joinable()) m_cast_sender_thread.join();
		}
	}
}

void PhoneServer::on_captured_frame(const uint8_t* data, size_t size)
{
	std::lock_guard<std::mutex> lock(m_latest_frame_mutex);
	m_latest_frame.assign(data, data + size);
	m_new_frame_available = true;
	m_latest_frame_cv.notify_one();
}

void PhoneServer::cast_sender_loop()
{
	std::vector<uint8_t> frame;

	while (m_casting_enabled)
	{
		{
			std::unique_lock<std::mutex> lock(m_latest_frame_mutex);
			m_latest_frame_cv.wait_for(lock, std::chrono::milliseconds(200),
				[this] { return m_new_frame_available || !m_casting_enabled; });

			if (!m_new_frame_available) continue;

			frame.swap(m_latest_frame);
			m_new_frame_available = false;
		}

		if (frame.empty()) continue;

		std::lock_guard<std::mutex> sockets_lock(m_ws_sockets_mutex);
		for (auto& socket : m_ws_sockets)
		{
			write_ws_binary_frame(*socket, frame.data(), frame.size());
		}
	}
}
