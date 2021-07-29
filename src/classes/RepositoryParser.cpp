#include "RepositoryParser.hpp"

RepositoryParser::RepositoryParser(std::string url) {
	this->url = url;
}

RepositoryParser::RepositoryParser(std::string url, std::string dist, std::string suite) {
	this->url = url;
	this->dist = dist;
	this->suite = suite;
}

void RepositoryParser::indexRepository() {
	if (dist.empty() && suite.empty()) {
		return indexSimpleRepository();
	} else {
		return indexDistributionRepository();
	}
}

void RepositoryParser::indexSimpleRepository() {
	if (url.size() == 0) {
		return;
	}

	if (url.ends_with("/")) {
		url.pop_back();
	}

	std::string content = fetchPackages(url);

	size_t start;
	size_t end = 0;
	std::vector<std::map<std::string, std::string>> packages;
	std::vector<std::future<std::map<std::string, std::string>>> package_threads;

	while ((start = content.find_first_not_of("\n\n", end)) != std::string::npos) {
		end = content.find("\n\n", start);

		// We want to do each package on a separate thread (hopefully BigBoss plays nicely)
		package_threads.push_back(std::async(&RepositoryParser::mapPackage, this, std::stringstream(content.substr(start, end - start))));
	}

	for (auto &result: package_threads) {
		packages.push_back(result.get());
	}

	// TODO: Parse and push to postgres
	for (const auto package: packages) {
		for (auto [ key, value ]: package) {
			std::cout << key << " - " << value << std::endl;
		}

		std::cout << std::endl;
	}
}

void RepositoryParser::indexDistributionRepository() {

}

std::string RepositoryParser::fetchPackages(std::string url) {
	try {
		// Automatically cleanup our request
		curlpp::Cleanup curl_cleanup;
		curlpp::Easy curl_request;
		std::ostringstream stream;

		curl_request.setOpt(curlpp::options::Url(url + "/Packages"));
		curl_request.setOpt(curlpp::options::WriteStream(&stream));
		curl_request.perform();
		return stream.str();
	} catch(curlpp::RuntimeError & e) {
		std::cout << e.what() << std::endl;
	} catch(curlpp::LogicError & e) {
		std::cout << e.what() << std::endl;
	}

	return nullptr;
}

std::map<std::string, std::string> RepositoryParser::mapPackage(std::stringstream stream) {
	std::map<std::string, std::string> control_map;
	std::string line, previousKey;

	while (std::getline(stream, line, '\n')) {
		if (line.size() == 0) {
			continue;
		}

		std::smatch matches;
		if (!std::regex_match(line, matches, std::regex("^(.*?): (.*)"))) {
			// There's a chance instead of multiline, some idiot just gave a key without value
			// Trim the string incase there may be a space after the colon
			size_t end = line.find_last_not_of(' ');
			end == std::string::npos ? line = "" : line = line.substr(0, end + 1);

			if (!line.ends_with(":")) {
				control_map[previousKey].append("\n" + line);
			}
		}

		if (matches.size() != 3) {
			continue;
		}

		control_map.insert(std::make_pair(matches[1], matches[2]));
		previousKey = matches[1];
	}

	return control_map;
}
