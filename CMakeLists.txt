cmake_minimum_required(VERSION 3.60)
project(
	cpp_request
	DESCRIPTION "A lightweight, dependency-free HTTP client library written from scratch in C/C++ to understand low-level network communication and TCP socket programming."
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/modules/output.cmake)
include(cmake/modules/warning.cmake)
include(cmake/modules/sanitizing.cmake)
include(cmake/modules/optimizing.cmake)

add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
add_subdirectory(${CMAKE_SOURCE_DIR}/benchmarks)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples)