if(NOT DEFINED CPP_REQUEST_SOURCE_DIR)
	message(FATAL_ERROR "CPP_REQUEST_SOURCE_DIR is required")
endif()

if(NOT DEFINED CPP_REQUEST_BINARY_DIR)
	message(FATAL_ERROR "CPP_REQUEST_BINARY_DIR is required")
endif()

set(install_prefix "${CPP_REQUEST_BINARY_DIR}/package-consumer-prefix")
set(consumer_build "${CPP_REQUEST_BINARY_DIR}/package-consumer-build")

file(REMOVE_RECURSE "${install_prefix}" "${consumer_build}")

set(install_command
	"${CMAKE_COMMAND}"
	--install "${CPP_REQUEST_BINARY_DIR}"
	--prefix "${install_prefix}")
if(DEFINED CPP_REQUEST_CONFIG AND NOT CPP_REQUEST_CONFIG STREQUAL "")
	list(APPEND install_command --config "${CPP_REQUEST_CONFIG}")
endif()

execute_process(
	COMMAND ${install_command}
	RESULT_VARIABLE install_result
	OUTPUT_VARIABLE install_output
	ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
	message(FATAL_ERROR
		"cpp_request install failed\n${install_output}\n${install_error}")
endif()

set(configure_command
	"${CMAKE_COMMAND}"
	-S "${CPP_REQUEST_SOURCE_DIR}/tests/package_consumer"
	-B "${consumer_build}"
	"-DCMAKE_PREFIX_PATH=${install_prefix}")
if(DEFINED CPP_REQUEST_CONFIG AND NOT CPP_REQUEST_CONFIG STREQUAL "")
	list(APPEND configure_command "-DCMAKE_BUILD_TYPE=${CPP_REQUEST_CONFIG}")
endif()

execute_process(
	COMMAND ${configure_command}
	RESULT_VARIABLE configure_result
	OUTPUT_VARIABLE configure_output
	ERROR_VARIABLE configure_error
)
if(NOT configure_result EQUAL 0)
	message(FATAL_ERROR
		"package consumer configure failed\n${configure_output}\n${configure_error}")
endif()

set(build_command "${CMAKE_COMMAND}" --build "${consumer_build}")
if(DEFINED CPP_REQUEST_CONFIG AND NOT CPP_REQUEST_CONFIG STREQUAL "")
	list(APPEND build_command --config "${CPP_REQUEST_CONFIG}")
endif()

execute_process(
	COMMAND ${build_command}
	RESULT_VARIABLE build_result
	OUTPUT_VARIABLE build_output
	ERROR_VARIABLE build_error
)
if(NOT build_result EQUAL 0)
	message(FATAL_ERROR
		"package consumer build failed\n${build_output}\n${build_error}")
endif()

set(test_command
	"${CMAKE_CTEST_COMMAND}"
	--test-dir "${consumer_build}"
	--output-on-failure)
if(DEFINED CPP_REQUEST_CONFIG AND NOT CPP_REQUEST_CONFIG STREQUAL "")
	list(APPEND test_command --build-config "${CPP_REQUEST_CONFIG}")
endif()

execute_process(
	COMMAND ${test_command}
	RESULT_VARIABLE test_result
	OUTPUT_VARIABLE test_output
	ERROR_VARIABLE test_error
)
if(NOT test_result EQUAL 0)
	message(FATAL_ERROR
		"package consumer test failed\n${test_output}\n${test_error}")
endif()
