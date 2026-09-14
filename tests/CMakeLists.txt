include(${CMAKE_SOURCE_DIR}/cmake/packages/google-test.cmake)
find_package(Threads REQUIRED)

add_executable(cpp_request_tests
	${CMAKE_CURRENT_SOURCE_DIR}/url_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/headers_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/request_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/request_serializer_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/native_socket_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/resolver_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcp_connection_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcp_io_test.cpp
)

target_include_directories(cpp_request_tests
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(cpp_request_tests
	PRIVATE
		cpp_request::internal
		GTest::gtest_main
		Threads::Threads
)

if(WIN32)
	target_link_libraries(cpp_request_tests PRIVATE ws2_32)
endif()

target_compile_features(cpp_request_tests PRIVATE cxx_std_17)

gtest_discover_tests(cpp_request_tests)
