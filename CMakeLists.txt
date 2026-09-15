cmake_minimum_required(VERSION 3.20)
project(
	cpp_request
	DESCRIPTION "A lightweight synchronous C++17 HTTP/1.1 client built on native sockets"
	VERSION 0.1.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CPP_REQUEST_BUILD_BENCHMARKS "Build cpp_request benchmarks" OFF)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/modules/output.cmake)
include(cmake/modules/warning.cmake)
include(cmake/modules/sanitizing.cmake)
include(cmake/modules/optimizing.cmake)

add_subdirectory(src)
if(BUILD_TESTING)
	add_subdirectory(tests)
endif()
if(CPP_REQUEST_BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()
add_subdirectory(examples)

set(CPP_REQUEST_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/cpp_request")

configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp_requestConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cpp_requestConfig.cmake
	INSTALL_DESTINATION ${CPP_REQUEST_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/cpp_requestConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMinorVersion
)

install(
	EXPORT cpp_requestTargets
	FILE cpp_requestTargets.cmake
	NAMESPACE cpp_request::
	DESTINATION ${CPP_REQUEST_INSTALL_CMAKEDIR}
)

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/cpp_requestConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/cpp_requestConfigVersion.cmake
	DESTINATION ${CPP_REQUEST_INSTALL_CMAKEDIR}
)
