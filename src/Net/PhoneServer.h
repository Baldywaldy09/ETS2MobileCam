/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Net/PhoneServer.h
/*-----------------------------------------*/

#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <asio.hpp>

// Hosts phone page (Net/phone_page.h) and its websocket endpoint on one shared port
class PhoneServer
{
public:
	static constexpr uint16_t DEFAULT_PORT = 47990;

	static PhoneServer* get();

	void start(uint16_t port = DEFAULT_PORT);
	void stop();

	// Stashes the latest JPEG frame from ScreenCapture and wakes cast_sender_loop
	void on_captured_frame(const uint8_t* data, size_t size);

private:
	PhoneServer() = default;
	PhoneServer(const PhoneServer&) = delete;
	PhoneServer& operator=(const PhoneServer&) = delete;

	struct parsed_request_t
	{
		std::string request_line;
		std::unordered_map<std::string, std::string> headers; // lowercase keys
	};

	void run_accept_loop();
	void handle_connection(const std::shared_ptr<asio::ip::tcp::socket>& socket);

	void handle_http_request(const std::shared_ptr<asio::ip::tcp::socket>& socket, const parsed_request_t& request);
	void handle_ws_connection(const std::shared_ptr<asio::ip::tcp::socket>& socket, const parsed_request_t& request);
	static bool perform_ws_handshake(asio::ip::tcp::socket& socket, const std::string& ws_key);
	void handle_message(const std::string& payload);

	void track_worker(std::thread thread);
	void join_workers();

	// Sends the latest captured frame
	void cast_sender_loop();

	std::atomic<bool> m_running{ false };

	std::unique_ptr<asio::io_context> m_io_context;
	std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor;
	std::thread m_accept_thread;

	std::mutex m_ws_sockets_mutex;
	std::vector<std::shared_ptr<asio::ip::tcp::socket>> m_ws_sockets;

	std::mutex m_workers_mutex;
	std::vector<std::thread> m_workers;

	std::atomic<bool> m_casting_enabled{ false };
	std::thread m_cast_sender_thread;
	std::mutex m_latest_frame_mutex;
	std::condition_variable m_latest_frame_cv;
	std::vector<uint8_t> m_latest_frame;
	bool m_new_frame_available{ false };
};
