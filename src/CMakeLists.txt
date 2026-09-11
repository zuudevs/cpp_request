add_library(internal 
	${CMAKE_CURRENT_SOURCE_DIR}/dummy.cpp
)

add_library(${PROJECT_NAME}::internal ALIAS internal)

target_include_directories(internal
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
)