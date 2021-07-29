#include <uWebSockets/App.h>
#include <nlohmann/json.hpp>
#include <date/date.h>
#include <regex>
#include <string>
#include <thread>
#include <future>

#include "IndexRepoCommand.hpp"

int main() {
	// All the WebSocket commands
	std::map<std::string, SocketCommand *> map = {
		{"index_repo", new IndexRepoCommand()}
	};

	uWS::App().ws<std::string>("/", {
		.compression = uWS::SHARED_COMPRESSOR,
		.maxPayloadLength = 16 * 1024 * 1024,
		.idleTimeout = 36,
		.maxBackpressure = 1 * 1024 * 1024,

		.upgrade = nullptr,
		.open = [](auto *ws) {
			nlohmann::json response = {
				{"status", "connected"},
				{"date", date::format("%F %T", std::chrono::system_clock::now())}
			};

			ws->send(response.dump(), uWS::OpCode::TEXT, true);
		},
		.message = [&map](auto *ws, std::string_view message, uWS::OpCode opCode) {
			// Why would we ever need anything other than text here
			if (opCode != uWS::OpCode::TEXT) {
				return;
			}

			try {
				// Parse our JSON output for commands
				nlohmann::json data = nlohmann::json::parse(message);

				// Make sure our command and payload actually exist
				if (!data.contains("command")) {
					nlohmann::json response = {
						{"status", "Error: Missing 'command' parameter"},
						{"date", date::format("%F %T", std::chrono::system_clock::now())}
					};

					ws->send(response.dump(), uWS::OpCode::TEXT, true);
					return;
				}

				if (!data.contains("payload")) {
					nlohmann::json response = {
						{"status", "Error: Missing 'payload' parameter"},
						{"date", date::format("%F %T", std::chrono::system_clock::now())}
					};

					ws->send(response.dump(), uWS::OpCode::TEXT, true);
					return;
				}

				// Let's make sure our command exists in the map first
				std::string command = data["command"].get<std::string>();
				if (map.find(command) == map.end()) {
					nlohmann::json response = {
						{"status", "Error: Command not found"},
						{"date", date::format("%F %T", std::chrono::system_clock::now())}
					};

					ws->send(response.dump(), uWS::OpCode::TEXT, true);
					return;
				}

				// Validate the payload before executing
				try {
					map[command]->validate(data["payload"]);
				} catch (std::exception &exc) {
					nlohmann::json response = {
						{"status", "Error: Payload Validation Failure"},
						{"date", date::format("%F %T", std::chrono::system_clock::now())},
						{"error", exc.what()}
					};

					ws->send(response.dump(), uWS::OpCode::TEXT, true);
					return;
				}

				// We can now try to execute our command here
				try {
					map[command]->execute(ws, data["payload"]);
				} catch (std::exception &exc) {
					nlohmann::json response = {
						{"status", "Error: Command Execution Failure"},
						{"date", date::format("%F %T", std::chrono::system_clock::now())},
						{"error", exc.what()}
					};

					ws->send(response.dump(), uWS::OpCode::TEXT, true);
					return;
				}
			} catch (nlohmann::detail::exception &exc) {
				nlohmann::json response = {
					{"status", "Error: Invalid JSON"},
					{"date", date::format("%F %T", std::chrono::system_clock::now())},
					{"error", exc.what()}
				};

				ws->send(response.dump(), uWS::OpCode::TEXT, true);
				return;
			} catch (std::exception &exc) {
				std::cout << exc.what() << std::endl;
			} catch (...) {
				std::cout << "error\n";
			}
		},
		.drain = [](auto */*ws*/) {
			/* Check getBufferedAmount here */
		},
		.ping = [](auto */*ws*/, std::string_view) {

		},
		.pong = [](auto */*ws*/, std::string_view) {

		},
		.close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
			/* We automatically unsubscribe from any topic here */
		}
	}).listen(9000, [](auto *listen_socket) {
		if (listen_socket) {
			std::cout << "Listening on port " << 9000 << std::endl;
		}
	}).run();
}
