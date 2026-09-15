include_guard(GLOBAL)

include("${PROJECT_SOURCE_DIR}/cmake/packages/google-test.cmake")
find_package(Threads REQUIRED)

function(cpp_request_add_test target)
	add_executable(${target} ${ARGN})

	target_include_directories(${target}
		PRIVATE
			${PROJECT_SOURCE_DIR}/src
	)

	target_link_libraries(${target}
		PRIVATE
			cpp_request::cpp_request
			GTest::gtest_main
			Threads::Threads
	)

	if(WIN32)
		target_link_libraries(${target} PRIVATE ws2_32)
	endif()

	target_compile_features(${target} PRIVATE cxx_std_17)
	cpp_request_configure_target(${target})
	gtest_discover_tests(${target})
endfunction()
