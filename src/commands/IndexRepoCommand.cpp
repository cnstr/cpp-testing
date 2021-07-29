#include "IndexRepoCommand.hpp"

void IndexRepoCommand::execute(uWS::WebSocket<false, true, std::string> *ws, nlohmann::json payload) {
	// Iterate through all of our repos and decide how we need to construct RepositoryParser
	for (auto iter = payload.begin(); iter != payload.end(); ++iter) {
		const auto object = iter.value();

		if (object.contains("dist") && object.contains("suite")) {

		} else {
			RepositoryParser parser(object["uri"].get<std::string>());
			parser.indexRepository();
		}
	}

	ws->send("hello", uWS::TEXT, true);
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