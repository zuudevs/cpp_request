#include <benchmark/benchmark.h>

#include "http/response_parser.hpp"

#include <cpp_request/request.hpp>

#include <cstdint>
#include <string>

namespace {

std::string make_response(std::size_t body_size) {
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Cache-Control: no-cache\r\n"
        "Server: cpp-request-benchmark\r\n";

    for (int index = 0; index < 24; ++index) {
        response += "X-Benchmark-" + std::to_string(index)
            + ": value-" + std::to_string(index) + "\r\n";
    }

    response += "Content-Length: " + std::to_string(body_size) + "\r\n\r\n";
    response.append(body_size, 'x');
    return response;
}

void BM_ResponseParser(benchmark::State& state) {
    const auto response = make_response(static_cast<std::size_t>(state.range(0)));

    for (auto _ : state) {
        cpp_request::detail::http::ResponseParser parser{cpp_request::Method::Get};
        auto result = parser.feed(response);
        if (!result || !parser.complete()) {
            state.SkipWithError("response parsing failed");
            break;
        }

        benchmark::DoNotOptimize(parser.response().status_code());
        benchmark::DoNotOptimize(parser.response().body().data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() * static_cast<std::int64_t>(response.size()));
}

BENCHMARK(BM_ResponseParser)
    ->Arg(0)
    ->Arg(4 * 1024)
    ->Arg(64 * 1024);

} // namespace
