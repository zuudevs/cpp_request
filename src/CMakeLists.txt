add_library(internal
	${CMAKE_CURRENT_SOURCE_DIR}/core/client.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/core/headers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/core/url.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/request_serializer.cpp
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

add_library(${PROJECT_NAME}::internal ALIAS internal)

target_include_directories(internal
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
)

target_compile_features(internal PUBLIC cxx_std_17)

if(WIN32)
	target_link_libraries(internal PRIVATE ws2_32)
endif()
