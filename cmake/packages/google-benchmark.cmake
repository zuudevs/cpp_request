find_package(benchmark CONFIG QUIET)

if(NOT benchmark_FOUND)
	include(FetchContent)

	set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
	set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

	FetchContent_Declare(
		googlebenchmark
		GIT_REPOSITORY https://github.com/google/benchmark.git
		GIT_TAG        v1.9.5
		GIT_SHALLOW    TRUE
	)
	FetchContent_MakeAvailable(googlebenchmark)
endif()
