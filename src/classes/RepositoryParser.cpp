#include "RepositoryParser.hpp"
#include <fstream>

RepositoryParser::RepositoryParser(std::string url) {
	this->url = url;
}

RepositoryParser::RepositoryParser(std::string url, std::string dist, std::string suite) {
	this->url = url;
	this->dist = dist;
	this->suite = suite;
}

int RepositoryParser::index_repository() {
	if (dist.empty() && suite.empty()) {
		return index_simple_repository();
	} else {
		return index_distribution_repository();
	}
}

int RepositoryParser::index_simple_repository() {
	if (url.size() == 0) {
		return -1;
	}

	if (url.ends_with("/")) {
		url.pop_back();
	}

	std::string content = fetch_packages(url);

	size_t start;
	size_t end = 0;
	std::vector<std::map<std::string, std::string>> packages;
	std::vector<std::future<std::map<std::string, std::string>>> package_threads;

	while ((start = content.find_first_not_of("\n\n", end)) != std::string::npos) {
		end = content.find("\n\n", start);

		// We want to do each package on a separate thread (hopefully BigBoss plays nicely)
		package_threads.push_back(std::async(&RepositoryParser::map_package, this, std::stringstream(content.substr(start, end - start))));
	}

	for (auto &result: package_threads) {
		packages.push_back(result.get());
	}

	return packages.size();
}

int RepositoryParser::index_distribution_repository() {
	return 0;
}

std::string RepositoryParser::fetch_packages(std::string url) {
	try {
		// ZSTD
		std::string zstd_buffer = fetch_packages_zstd(url);
		if (!zstd_buffer.empty()) return zstd_buffer;

		// BZip2
		std::string bzip2_buffer = fetch_packages_bzip2(url);
		if (!bzip2_buffer.empty()) return bzip2_buffer;

		// GZip
		std::string gzip_buffer = fetch_packages_gzip(url);
		if (!gzip_buffer.empty()) return gzip_buffer;
	} catch (std::exception &exc) {
		std::cout << exc.what() << std::endl;
	}

	return std::string();
}

std::map<std::string, std::string> RepositoryParser::map_package(std::stringstream stream) {
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

std::string RepositoryParser::fetch_packages_gzip(std::string url) {
	try {
		// Write the response data to a file and decompress it
		std::string file_name = curl_generic_url(url + "/Packages.gz");
		std::ifstream cache_file(("/tmp/" + file_name).c_str());
		if (!cache_file.is_open()) {
			throw std::runtime_error(url + ": Failed to open compressed file - GZip");
		}

		// Converts our ifstream to a string using streambuf iterators
		std::string buffer_data = std::string((std::istreambuf_iterator<char>(cache_file)), std::istreambuf_iterator<char>());
		std::string output_data;
		size_t max_size(16384);
		z_stream inflate_stream;

		// zLIB why do you feel a need
		inflate_stream.zalloc = Z_NULL;
		inflate_stream.zfree = Z_NULL;
		inflate_stream.opaque = Z_NULL;
		inflate_stream.next_in = Z_NULL;
		inflate_stream.avail_in = 0;

		// windowBits 15
		// ENABLE_ZLIB_GZIP 32
		int status = inflateInit2(&inflate_stream, 15 | 32);
		if (status < 0) {
			throw std::runtime_error(url + ": GZip failed to initialize zLIB inflate");
		}

		std::size_t inflated_size = 0;
		std::size_t buffer_size = buffer_data.size();

		// Because of zLIB's weird pointer magic, we need to reinterpret this as a pointer first
		inflate_stream.next_in = reinterpret_cast<z_const Bytef *>(buffer_data.data());
		if (buffer_size > max_size || (buffer_size * 2) > max_size) {
			inflateEnd(&inflate_stream);
			throw std::runtime_error(url + ": GZip inflate may use more memory due to size");
		}

		inflate_stream.avail_in = static_cast<unsigned int>(buffer_size);

		do {
			// The string buffer needs a reference of it's next resize so it may expand to fit data
			std::size_t next_resize = inflated_size + 2 * buffer_size;

			if (next_resize > max_size) {
				inflateEnd(&inflate_stream);
				throw std::runtime_error(url + ": GZip decompression reference size exceeds max size");
			}

			output_data.resize(next_resize);

			// Expand the size of our zLIB stream too so it can continue inflating
			inflate_stream.avail_out = static_cast<unsigned int>(2 * buffer_size);
			inflate_stream.next_out = reinterpret_cast<Bytef *>(&output_data[0] + inflated_size);

			int inflate_status = inflate(&inflate_stream, Z_FINISH);

			// If this happens we didn't reach Z_FINISH and the buffer is just dead
			if (inflate_status != Z_STREAM_END && inflate_status != Z_OK && inflate_status != Z_BUF_ERROR) {
				std::string error_message = inflate_stream.msg;
				inflateEnd(&inflate_stream);

				throw std::runtime_error(url + ": GZip finish error: zLIB - " + error_message);
			}

			inflated_size += (2 * buffer_size - inflate_stream.avail_out);
		} while (inflate_stream.avail_out == 0);

		inflateEnd(&inflate_stream);
		output_data.resize(inflated_size);

		return output_data;
	} catch (std::exception &exc) {
		std::cout << exc.what() << std::endl;
		return std::string();
	}
}

std::string RepositoryParser::fetch_packages_zstd(std::string url) {
	try {
		// Write the response data to a file and decompress it
		std::string file_name = curl_generic_url(url + "/Packages.zst");
		std::string out_cache_name = ("/tmp/" + file_name + "_decompressed");

		FILE *const file_in = fopen(("/tmp/" + file_name).c_str(), "rb");
		FILE *const file_out = fopen(out_cache_name.c_str(), "wb+");

		if (!file_in) {
			fclose(file_out);
			throw std::runtime_error(url + ": Failed to open compressed file - ZSTD");
		}

		if (!file_out) {
			fclose(file_in);
			throw std::runtime_error(url + ": Failed to create decompressed file - ZSTD");
		}

		// Streams don't require a set size, so we use ZSTD's allocated size
		size_t const buffer_in_size = ZSTD_DStreamInSize();
		size_t const buffer_out_size = ZSTD_DStreamOutSize();

		void *const buffer_in = malloc(buffer_in_size);
		void *const buffer_out = malloc(buffer_out_size);

		ZSTD_DCtx *const dict_ctx = ZSTD_createDCtx();
		if (dict_ctx == NULL) {
			fclose(file_in);
			free(buffer_in);
			free(buffer_out);

			throw std::runtime_error(url + ": Failed to create ZSTD Dictionary Context");
		}

		size_t read_size;
		size_t last_status = 0;
		size_t const pending_read = buffer_in_size;

		// We have to basically read every byte at a time and decompress/write it to a buffer
		while ((read_size = fread(buffer_in, 1, pending_read, file_in))) {
			ZSTD_inBuffer input = {
				buffer_in,
				read_size,
				0
			};

			while (input.pos < input.size) {
				ZSTD_outBuffer output = {
					buffer_out,
					buffer_out_size,
					0
				};

				size_t const status = ZSTD_decompressStream(dict_ctx, &output, &input);
				if (ZSTD_isError(status)) {
					ZSTD_freeDCtx(dict_ctx);
					fclose(file_in);
					fclose(file_out);
					free(buffer_in);
					free(buffer_out);

					throw std::runtime_error(url + ": ZSTD Decompression error - " + ZSTD_getErrorName(status));
				}

				fwrite(buffer_out, 1, output.pos, file_out);
				last_status = status;
			}
		}

		// If the last status was not okay that means decompression failed somewhere
		if (last_status != 0) {
			ZSTD_freeDCtx(dict_ctx);
			fclose(file_in);
			fclose(file_out);
			free(buffer_in);
			free(buffer_out);

			throw std::runtime_error(url + ": EOF before end of stream - " + ZSTD_getErrorName(last_status));
		}

		ZSTD_freeDCtx(dict_ctx);
		fclose(file_in);
		fclose(file_out);
		free(buffer_in);
		free(buffer_out);

		std::ifstream cache_file(out_cache_name);
		if (!cache_file.is_open()) {
			throw std::runtime_error(url + ": ZSTD cache file not opened");
		}

		// Converts our ifstream to a string using streambuf iterators
		std::string cache_data = std::string((std::istreambuf_iterator<char>(cache_file)), std::istreambuf_iterator<char>());

		// Delete our cache file and then return
		std::remove(out_cache_name.c_str());
		return cache_data;
	} catch (std::exception &exc) {
		std::cout << exc.what() << std::endl;
		return std::string();
	}
}

std::string RepositoryParser::fetch_packages_bzip2(std::string url) {
	try {
		// Write the response data to a file and decompress it
		std::string file_name = curl_generic_url(url + "/Packages.bz2");
		std::string out_cache_name = ("/tmp/" + file_name + "_decompressed");

		FILE *const file_in = fopen(("/tmp/" + file_name).c_str(), "rb");
		FILE *const file_out = fopen(out_cache_name.c_str(), "wb+");
		char output_buffer[4096];

		if (!file_in) {
			fclose(file_out);
			throw std::runtime_error(url + ": Failed to open compressed file - BZip2");
		}

		if (!file_out) {
			fclose(file_in);
			throw std::runtime_error(url + ": Failed to create decompressed file - BZip2");
		}

		int status = 0;
		BZFILE *bzip_handle = BZ2_bzReadOpen(&status, file_in, 0, 0, NULL, 0);

		if (status != BZ_OK) {
			BZ2_bzReadClose(&status, bzip_handle);
			fclose(file_out);
			fclose(file_in);

			throw std::runtime_error(url + ": Failed to transfer BZip2 file to BZFILE");
		}

		do {
			int read_position = BZ2_bzRead(&status, bzip_handle, output_buffer, sizeof output_buffer);
			if (status == BZ_OK || status == BZ_STREAM_END) {
				fwrite(output_buffer, 1, read_position, file_out);
			}
		} while (status == BZ_OK);

		if (status != BZ_STREAM_END) {
			BZ2_bzReadClose(&status, &bzip_handle);
			fclose(file_out);
			fclose(file_in);

			throw std::runtime_error(url + ": EOF before end of stream - " + BZ2_bzerror(&bzip_handle, &status));
		}

		fclose(file_out);
		fclose(file_in);

		std::ifstream cache_file(out_cache_name);
		if (!cache_file.is_open()) {
			throw std::runtime_error(url + ": BZip2 cache file not opened");
		}

		// Converts our ifstream to a string using streambuf iterators
		std::string cache_data = std::string((std::istreambuf_iterator<char>(cache_file)), std::istreambuf_iterator<char>());

		// Delete our cache file and then return
		std::remove(out_cache_name.c_str());
		return cache_data;
	} catch (std::exception &exc) {
		std::cout << exc.what() << std::endl;
		return std::string();
	}
}

std::string RepositoryParser::curl_generic_url(std::string url) {
	try {
		curlpp::Easy curl_handle;
		std::ostringstream response_stream;

		// Headers are required for certain repositories (Dynastic)
		std::list<std::string> headers;
		headers.push_back("X-Firmware: 2.0");
		headers.push_back("X-Machine: iPhone13,1");
		headers.push_back("Cache-Control: no-cache");
		headers.push_back("X-Unique-ID: canister-v2-unique-device-identifier");
		std::string user_agent = "Canister/2.0 (+https://canister.me/go/ua)";

		curl_handle.setOpt(curlpp::options::Url(url));
		curl_handle.setOpt(curlpp::options::Timeout(10));
		curl_handle.setOpt(curlpp::options::HttpHeader(headers));
		curl_handle.setOpt(curlpp::options::UserAgent(user_agent));
		curl_handle.setOpt(curlpp::options::WriteStream(&response_stream));
		curl_handle.perform();

		// If the status code isn't 200 then we just return a blank string
		// The blank string is ignored by the individual fetch methods
		long status_code = curlpp::infos::ResponseCode::get(curl_handle);
		if (status_code != 200) {
			throw std::runtime_error(url + ": Status Code - " + std::to_string(status_code));
		}

		std::string response_data = response_stream.str();
		if (response_data.empty()) {
			throw std::runtime_error(url + ": Empty response buffer");
		}

		// We need to normalize filenames for the FS by removing slashes and dots
		std::string file_name = url.substr(url.find("://") + 3);
		std::transform(file_name.begin(), file_name.end(), file_name.begin(), [](char value) {
			if (value == '.' || value == '/') {
				return '_';
			}

			return value;
		});

		// Write the response data to the file and then return the file name
		std::ofstream out("/tmp/" + file_name, std::ios::binary | std::ios::out);
		out << response_data;
		out.flush();
		out.close();

		return file_name;
	} catch (std::exception &exc) {
		throw std::runtime_error(exc.what());
	}
}
