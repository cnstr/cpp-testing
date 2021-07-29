#include "SocketCommand.hpp"
#include "RepositoryParser.hpp"

class IndexRepoCommand: public SocketCommand {
public:
	IndexRepoCommand() {};
	~IndexRepoCommand() {};

	void execute(uWS::WebSocket<false, true, std::string> *ws, nlohmann::json payload) override;
	nlohmann::json schema() override;
};
