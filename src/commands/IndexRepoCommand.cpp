#include "IndexRepoCommand.hpp"

void IndexRepoCommand::execute(uWS::WebSocket<false, true, std::string> *ws, nlohmann::json payload) {
	// Iterate through all of our repos and decide how we need to construct RepositoryParser
	for (auto iter = payload.begin(); iter != payload.end(); ++iter) {
		const auto object = iter.value();

		if (object.contains("dist") && object.contains("suite")) {
			std::string uri = object["uri"].get<std::string>();
			std::string dist = object["dist"].get<std::string>();
			std::string suite = object["suite"].get<std::string>();
			RepositoryParser parser(uri, dist, suite);

			int packageCount = parser.index_repository();
			nlohmann::json response = {
				{"status", "Repository Completed"},
				{"date", date::format("%F %T", std::chrono::system_clock::now())},
				{"package_count", packageCount},
				{"repository_url", {
					{"uri", object["uri"]},
					{"dist", object["dist"]},
					{"suite", object["suite"]}
				}}
			};

			ws->send(response.dump(), uWS::OpCode::TEXT, true);
		} else {
			std::string uri = object["uri"].get<std::string>();
			RepositoryParser parser(uri);
			int packageCount = parser.index_repository();
			nlohmann::json response = {
				{"status", "Repository Completed"},
				{"date", date::format("%F %T", std::chrono::system_clock::now())},
				{"package_count", packageCount},
				{"repository_url", object["uri"]}
			};

			ws->send(response.dump(), uWS::OpCode::TEXT, true);
		}
	}
}

nlohmann::json IndexRepoCommand::schema() {
	return R"(
{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"$ref": "#/definitions/IndexRepoSchema",
	"definitions": {
		"IndexRepoSchema": {
			"type": "array",
			"items": {
				"type": "object",
				"properties": {
					"uri": {
						"type": "string"
					},
					"slug": {
						"type": "string"
					},
					"ranking": {
						"type": "number",
						"minimum": 1,
						"maximum": 5
					},
					"aliases": {
						"type": "array",
						"items": {
							"type": "string"
						}
					},
					"dist": {
						"type": "string"
					},
					"suite": {
						"type": "string"
					}
				},
				"additionalProperties": false,
				"required": [
					"uri",
					"slug",
					"ranking"
				]
			}
		}
	}
}
	)"_json;
}
