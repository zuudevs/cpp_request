add_library(cpp_request
	${CMAKE_CURRENT_SOURCE_DIR}/core/client.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/core/headers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/core/url.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/request_serializer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/redirect.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/chunked_decoder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/response_parser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/native_socket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/network_runtime.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/socket_mode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/socket_io.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/socket_connect.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/resolver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/tcp_connection.cpp
)

set_target_properties(cpp_request PROPERTIES
	EXPORT_NAME cpp_request
	OUTPUT_NAME cpp_request
)

add_library(cpp_request::cpp_request ALIAS cpp_request)
add_library(cpp_request::internal ALIAS cpp_request)

target_include_directories(cpp_request
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
		${PROJECT_SOURCE_DIR}/src
)

target_compile_features(cpp_request PUBLIC cxx_std_17)

if(WIN32)
	target_link_libraries(cpp_request PRIVATE ws2_32)
endif()

cpp_request_configure_target(cpp_request)
