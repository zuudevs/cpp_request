cmake_minimum_required(VERSION 3.21)

project(
	cpp_request
	VERSION 0.9.0
	DESCRIPTION "A lightweight synchronous C++17 HTTP/1.1 client built on native sockets"
	LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

include(options)
cpp_request_setup_options()

include(project-settings)
cpp_request_setup_project()

add_subdirectory(src)

if(CPP_REQUEST_BUILD_TESTS)
	include(testing)
	add_subdirectory(tests)
endif()

if(CPP_REQUEST_BUILD_BENCHMARKS)
	include(benchmarking)
	add_subdirectory(benchmarks)
endif()

if(CPP_REQUEST_BUILD_EXAMPLES)
	include(examples)
	add_subdirectory(examples)
endif()

if(CPP_REQUEST_ENABLE_INSTALL)
	include(install)
	cpp_request_configure_install()
endif()

if(PROJECT_IS_TOP_LEVEL AND CPP_REQUEST_ENABLE_CLANG_FORMAT)
	include("${PROJECT_SOURCE_DIR}/cmake/tools/clang-format.cmake")
	cpp_request_add_clang_format_targets()
endif()

if(PROJECT_IS_TOP_LEVEL AND CPP_REQUEST_ENABLE_DOXYGEN)
	include("${PROJECT_SOURCE_DIR}/cmake/tools/doxygen.cmake")
	cpp_request_add_doxygen_target(cpp_request_docs)
endif()
