#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <uWebSockets/App.h>
#include <string>

class SocketCommand {
public:
	virtual void execute(uWS::WebSocket<false, true, std::string> *ws, nlohmann::json payload) = 0;
	virtual nlohmann::json schema() = 0;

	void validate(nlohmann::json payload) {
		validator.set_root_schema(schema());
		validator.validate(payload);
	}

	SocketCommand() {};
	~SocketCommand() {};
	SocketCommand(const SocketCommand &command) {
		name = command.name;
	}

protected:
	std::string name;

private:
	nlohmann::json_schema::json_validator validator;
};
