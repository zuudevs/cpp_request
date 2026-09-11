cmake_minimum_required(VERSION 3.60)
project(
	cpp_request
	DESCRIPTION "A lightweight, modern C++ HTTP/HTTPS client library designed for low-level performance and clean API ergonomics. Built from the ground up to provide a zero-heavy-dependency network solution, it features an intuitive fluent interface (builder pattern) for crafting requests, an integrated URL parser, and a cross-platform socket wrapper. Ideal for systems engineering and lightweight applications requiring direct control over network transport and memory management"
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