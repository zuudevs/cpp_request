add_library(internal
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

set_target_properties(internal PROPERTIES
	EXPORT_NAME cpp_request
	OUTPUT_NAME cpp_request
)

add_library(${PROJECT_NAME}::internal ALIAS internal)
add_library(${PROJECT_NAME}::cpp_request ALIAS internal)

target_include_directories(internal
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
		${PROJECT_SOURCE_DIR}/src
)

target_compile_features(internal PUBLIC cxx_std_17)

if(WIN32)
	target_link_libraries(internal PRIVATE ws2_32)
endif()

install(
	TARGETS internal
	EXPORT cpp_requestTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/include/cpp_request
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp"
)
