#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Easy.hpp>
#include <bzlib.h>
#include <lzma.h>
#include <zstd.h>
#include <zlib.h>

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
	int indexRepository();

private:
	std::string url, dist, suite;
	std::string fetchPackages(std::string url);
	std::map<std::string, std::string> mapPackage(std::stringstream stream);

	int indexSimpleRepository();
	int indexDistributionRepository();

	std::string fetch_packages_xz(std::string url);
	std::string fetch_packages_gzip(std::string url);
	std::string fetch_packages_zstd(std::string url);
	std::string fetch_packages_bzip2(std::string url);
	std::string curl_generic_url(std::string url);
};
