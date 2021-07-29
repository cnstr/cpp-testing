#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <sstream>
#include <thread>
#include <future>
#include <regex>

class RepositoryParser {
public:
	RepositoryParser(std::string url);
	RepositoryParser(std::string url, std::string dist, std::string suite);
	RepositoryParser() {};
	~RepositoryParser() {};
	void indexRepository();

private:
	std::string url, dist, suite;
	std::string fetchPackages(std::string url);
	std::map<std::string, std::string> mapPackage(std::stringstream stream);

	void indexSimpleRepository();
	void indexDistributionRepository();
};
