add_library(internal
	${CMAKE_CURRENT_SOURCE_DIR}/dummy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/url.cpp
)

add_library(${PROJECT_NAME}::internal ALIAS internal)

target_include_directories(internal
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
)

target_compile_features(internal PUBLIC cxx_std_17)
