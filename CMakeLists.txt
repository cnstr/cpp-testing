cmake_minimum_required (VERSION 3.5.1)
project(canister-core)

set(CMAKE_C_FLAGS_DEBUG "${CXX_COMPILATION_FLAGS} -g -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CXX_COMPILATION_FLAGS} -ldl")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CXX_COMPILATION_FLAGS} -std=gnu99 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILATION_FLAGS} -ldl -std=c++2a -Wno-deprecated-declarations -Wno-reorder")

set(SOURCES
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/commands/IndexRepoCommand.cpp
	${PROJECT_SOURCE_DIR}/src/classes/RepositoryParser.cpp
)

set(HEADERS
	${PROJECT_SOURCE_DIR}/include/IndexRepoCommand.hpp
	${PROJECT_SOURCE_DIR}/include/RepositoryParser.hpp
	${PROJECT_SOURCE_DIR}/include/SocketCommand.hpp
)

find_library(LIB_SOCKETS NAMES uSockets.a)
find_library(LIB_DATE NAMES libdate-tz.a)
find_library(LIB_CURLPP NAMES libcurlpp.a)
find_package(nlohmann_json_schema_validator REQUIRED)

add_executable(canister-core ${HEADERS} ${SOURCES})
target_include_directories(canister-core PUBLIC include)
target_link_libraries(canister-core
	nlohmann_json_schema_validator
	${LIB_SOCKETS}
	${LIB_CURLPP}
	${LIB_DATE}
	pthread
	crypto
	zstd
	curl
	lzma
	ssl
	bz2
	z
)

set_target_properties(canister-core PROPERTIES
	CMAKE_CXX_STANDARD 20
	CMAKE_CXX_STANDARD_REQUIRED YES
)
