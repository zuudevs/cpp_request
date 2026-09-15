cmake_minimum_required(VERSION 3.20)
project(cpp_request_package_consumer LANGUAGES CXX)

find_package(cpp_request CONFIG REQUIRED)

add_executable(cpp_request_package_consumer main.cpp)
target_link_libraries(
	cpp_request_package_consumer
	PRIVATE
		cpp_request::cpp_request
)
target_compile_features(cpp_request_package_consumer PRIVATE cxx_std_17)

enable_testing()
add_test(NAME package_consumer_runs COMMAND cpp_request_package_consumer)
