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

set(CPP_REQUEST_BENCHMARK_TARGETS
	cpp_request_bench_request_serializer
	cpp_request_bench_response_parser
	cpp_request_bench_chunked_decoder
	cpp_request_bench_loopback_client
	cpp_request_bench_connection_reuse
)

add_custom_target(cpp_request_benchmarks
	DEPENDS ${CPP_REQUEST_BENCHMARK_TARGETS}
)

add_custom_target(cpp_request_benchmark_smoke
	COMMAND $<TARGET_FILE:cpp_request_bench_request_serializer>
		--benchmark_min_time=0.01
		--benchmark_repetitions=1
	COMMAND $<TARGET_FILE:cpp_request_bench_response_parser>
		--benchmark_min_time=0.01
		--benchmark_repetitions=1
	COMMAND $<TARGET_FILE:cpp_request_bench_chunked_decoder>
		--benchmark_min_time=0.01
		--benchmark_repetitions=1
	COMMAND $<TARGET_FILE:cpp_request_bench_loopback_client>
		--benchmark_min_time=0.01
		--benchmark_repetitions=1
	COMMAND $<TARGET_FILE:cpp_request_bench_connection_reuse>
		--benchmark_min_time=0.01
		--benchmark_repetitions=1
	DEPENDS ${CPP_REQUEST_BENCHMARK_TARGETS}
	USES_TERMINAL
)
