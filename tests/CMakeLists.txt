cpp_request_add_test(cpp_request_tests
	${CMAKE_CURRENT_SOURCE_DIR}/result_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/url_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/headers_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/request_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/request_serializer_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/query_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/redirect_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/chunked_decoder_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/response_parser_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/response_limits_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/response_upgrade_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/client_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/client_reuse_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/client_redirect_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/native_socket_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/resolver_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcp_connection_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tcp_io_test.cpp
)

set(CPP_REQUEST_PUBLIC_HEADERS
	client.hpp
	error.hpp
	headers.hpp
	request.hpp
	response.hpp
	response_limits.hpp
	result.hpp
	url.hpp
)

set(CPP_REQUEST_PUBLIC_HEADER_SOURCES)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/public_headers")

foreach(header IN LISTS CPP_REQUEST_PUBLIC_HEADERS)
	string(REPLACE ".hpp" "" header_stem "${header}")
	set(
		source
		"${CMAKE_CURRENT_BINARY_DIR}/public_headers/${header_stem}.cpp"
	)
	file(WRITE "${source}" "#include <cpp_request/${header}>\n")
	list(APPEND CPP_REQUEST_PUBLIC_HEADER_SOURCES "${source}")
endforeach()

add_library(
	cpp_request_public_headers_compile
	OBJECT
	${CPP_REQUEST_PUBLIC_HEADER_SOURCES}
)
target_include_directories(
	cpp_request_public_headers_compile
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
)
target_compile_features(cpp_request_public_headers_compile PRIVATE cxx_std_17)
cpp_request_apply_warnings(cpp_request_public_headers_compile)

add_test(
	NAME PackageConsumer.InstallAndUse
	COMMAND
		${CMAKE_COMMAND}
		-DCPP_REQUEST_SOURCE_DIR=${PROJECT_SOURCE_DIR}
		-DCPP_REQUEST_BINARY_DIR=${PROJECT_BINARY_DIR}
		-DCPP_REQUEST_CONFIG=$<CONFIG>
		-P ${CMAKE_CURRENT_SOURCE_DIR}/package_consumer_test.cmake
)
