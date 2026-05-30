cmake_minimum_required(VERSION 3.20)

project(
	cpp_request
	VERSION 1.0
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(CORE_SOURCES
	${CMAKE_SOURCE_DIR}/src/core/socket.cpp
	${CMAKE_SOURCE_DIR}/src/transport/winsock_init.cpp
	${CMAKE_SOURCE_DIR}/src/platform/winsock.cpp
	${CMAKE_SOURCE_DIR}/src/client/client.cpp
	${CMAKE_SOURCE_DIR}/src/http/request.cpp
	${CMAKE_SOURCE_DIR}/src/http/response.cpp
)

add_library(cpp_request STATIC
	${CORE_SOURCES}
)

add_executable(cpp_request_app
	${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(cpp_request_app PRIVATE
	cpp_request
)

target_include_directories(cpp_request
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
)

target_include_directories(cpp_request_app
	PRIVATE
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
	target_compile_options(cpp_request PRIVATE
		$<$<CONFIG:Release>:/O2 /GL /DNDEBUG>
	)
	target_link_options(cpp_request PRIVATE
		$<$<CONFIG:Release>:/LTCG>
	)
else()
	target_compile_options(cpp_request PRIVATE
		$<$<CONFIG:Release>:-O3 -flto -DNDEBUG>
	)
	target_link_options(cpp_request PRIVATE
		$<$<CONFIG:Release>:-flto>
	)
endif()


message("=========================================")
message("Project Name    : ${PROJECT_NAME}")
message("Project Version : ${PROJECT_VERSION}")
message("C++ Standard    : ${CMAKE_CXX_STANDARD}")
message("=========================================")