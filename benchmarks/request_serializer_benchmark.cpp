#include <benchmark/benchmark.h>

#include "http/request_serializer.hpp"

#include <cpp_request/request.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

void BM_RequestSerializer(benchmark::State& state) {
    const std::string body(static_cast<std::size_t>(state.range(0)), 'x');

    cpp_request::Request request{
        cpp_request::Method::Post,
        "http://127.0.0.1:8080/api/v1/items?existing=1"};
    request.headers().add("Accept", "application/json");
    request.headers().add("Content-Type", "application/json");
    request.headers().add("X-Request-Id", "benchmark-request");
    request.add_query_param("search", "low level C++");
    request.add_query_param("tag", "network/http");
    request.set_body(body);

    for (auto _ : state) {
        auto result = cpp_request::detail::http::serialize_request(request);
        if (!result) {
            state.SkipWithError("request serialization failed");
            break;
        }

        benchmark::DoNotOptimize(result.value().head.data());
        benchmark::DoNotOptimize(result.value().body.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() * static_cast<std::int64_t>(body.size()));
}

BENCHMARK(BM_RequestSerializer)
    ->Arg(0)
    ->Arg(1024)
    ->Arg(16 * 1024);

} // namespace
