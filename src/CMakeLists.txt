add_library(internal
	${CMAKE_CURRENT_SOURCE_DIR}/dummy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/url.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/native_socket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/platform/network_runtime.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/resolver.cpp
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
