include("${CMAKE_SOURCE_DIR}/cmake/packages/google-test.cmake")

function(add_test_target name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} 
		PRIVATE 
			GTest::gtest_main
			${PROJECT_NAME}::internal
	)
	gtest_discover_tests(${name})
endfunction()