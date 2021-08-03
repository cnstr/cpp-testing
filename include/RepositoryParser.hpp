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
	int index_repository();

private:
	std::string url, dist, suite;
	std::string fetch_packages(std::string url);
	std::map<std::string, std::string> map_package(std::stringstream stream);

	int index_simple_repository();
	int index_distribution_repository();

	std::string fetch_packages_xz(std::string url);
	std::string fetch_packages_gzip(std::string url);
	std::string fetch_packages_zstd(std::string url);
	std::string fetch_packages_bzip2(std::string url);
	std::string fetch_packages_normal(std::string url);
	std::string curl_generic_url(std::string url);
};
