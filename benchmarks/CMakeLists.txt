include(${PROJECT_SOURCE_DIR}/cmake/packages/google-benchmark.cmake)
find_package(Threads REQUIRED)

function(cpp_request_add_benchmark target source)
	add_executable(${target} ${source})
	target_include_directories(${target}
		PRIVATE
			${PROJECT_SOURCE_DIR}/src
			${CMAKE_CURRENT_SOURCE_DIR}
	)
	target_link_libraries(${target}
		PRIVATE
			cpp_request::internal
			benchmark::benchmark_main
			Threads::Threads
	)
	target_compile_features(${target} PRIVATE cxx_std_17)

	if(WIN32)
		target_link_libraries(${target} PRIVATE ws2_32)
	endif()
endfunction()

cpp_request_add_benchmark(
	cpp_request_bench_request_serializer
	request_serializer_benchmark.cpp
)
cpp_request_add_benchmark(
	cpp_request_bench_response_parser
	response_parser_benchmark.cpp
)
cpp_request_add_benchmark(
	cpp_request_bench_chunked_decoder
	chunked_decoder_benchmark.cpp
)
cpp_request_add_benchmark(
	cpp_request_bench_loopback_client
	loopback_client_benchmark.cpp
)
cpp_request_add_benchmark(
	cpp_request_bench_connection_reuse
	connection_reuse_benchmark.cpp
)

add_custom_target(cpp_request_benchmarks
	DEPENDS
		cpp_request_bench_request_serializer
		cpp_request_bench_response_parser
		cpp_request_bench_chunked_decoder
		cpp_request_bench_loopback_client
		cpp_request_bench_connection_reuse
)
