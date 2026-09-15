#include <benchmark/benchmark.h>

#include "http/chunked_decoder.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace {

std::string make_chunked_body(std::size_t body_size, std::size_t chunk_size) {
    std::string encoded;
    std::size_t remaining = body_size;

    while (remaining != 0) {
        const std::size_t amount = remaining < chunk_size ? remaining : chunk_size;
        std::ostringstream line;
        line << std::hex << amount;
        encoded += line.str();
        encoded += "\r\n";
        encoded.append(amount, 'x');
        encoded += "\r\n";
        remaining -= amount;
    }

    encoded += "0\r\nX-Benchmark: done\r\n\r\n";
    return encoded;
}

void BM_ChunkedDecoder(benchmark::State& state) {
    const std::size_t body_size = static_cast<std::size_t>(state.range(0));
    const std::string encoded = make_chunked_body(body_size, 4096);
    std::string input;
    std::string output;

    for (auto _ : state) {
        state.PauseTiming();
        input = encoded;
        output.clear();
        output.reserve(body_size);
        state.ResumeTiming();

        cpp_request::detail::http::ChunkedDecoder decoder;
        auto result = decoder.process(input, output);
        if (!result || output.size() != body_size) {
            state.SkipWithError("chunked decoding failed");
            break;
        }

        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() * static_cast<std::int64_t>(body_size));
}

BENCHMARK(BM_ChunkedDecoder)
    ->Arg(4 * 1024)
    ->Arg(64 * 1024)
    ->Arg(1024 * 1024);

} // namespace
