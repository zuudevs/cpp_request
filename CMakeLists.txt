cmake_minimum_required(VERSION 3.20)
project(
	cpp_request
	VERSION 1.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Core library source files
set(CORE_SOURCES
	${CMAKE_SOURCE_DIR}/src/core/socket.cpp
	${CMAKE_SOURCE_DIR}/src/transport/winsock_init.cpp
	${CMAKE_SOURCE_DIR}/src/platform/winsock.cpp
	${CMAKE_SOURCE_DIR}/src/client/client.cpp
	${CMAKE_SOURCE_DIR}/src/http/request.cpp
	${CMAKE_SOURCE_DIR}/src/http/response.cpp
)

# Main executable
add_executable(${PROJECT_NAME}
	${CMAKE_SOURCE_DIR}/src/main.cpp
	${CORE_SOURCES}
)

# Public include directory (for users)
target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

# Internal include directory (for implementation)
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/src
)

message("")
message("")
message("Project Name    : ${PROJECT_NAME}")
message("Project Version : ${PROJECT_VERSION}")
message("C++ Standard    : ${CMAKE_CXX_STANDARD}")
message("")
message("")