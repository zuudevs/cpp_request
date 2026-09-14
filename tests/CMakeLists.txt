include(${CMAKE_SOURCE_DIR}/cmake/packages/google-test.cmake)

add_executable(cpp_request_tests
	${CMAKE_CURRENT_SOURCE_DIR}/url_test.cpp
)

target_link_libraries(cpp_request_tests
	PRIVATE
		cpp_request::internal
		GTest::gtest_main
)

target_compile_features(cpp_request_tests PRIVATE cxx_std_17)

gtest_discover_tests(cpp_request_tests)
