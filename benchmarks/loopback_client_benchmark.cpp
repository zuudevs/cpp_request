#include <benchmark/benchmark.h>

#include "loopback_benchmark_server.hpp"

#include <cpp_request/client.hpp>

#include <chrono>
#include <cstdint>

namespace {

void configure(cpp_request::Client& client) {
    client.set_connect_timeout(std::chrono::seconds{2});
    client.set_read_timeout(std::chrono::seconds{2});
    client.set_write_timeout(std::chrono::seconds{2});
}

void BM_LoopbackClient(benchmark::State& state) {
    const std::size_t body_size = static_cast<std::size_t>(state.range(0));
    cpp_request::bench::LoopbackBenchmarkServer server{body_size};
    if (!server.valid()) {
        state.SkipWithError("failed to create loopback server");
        return;
    }

    cpp_request::Client client;
    configure(client);
    const std::string url = server.url("/benchmark");

    auto warmup = client.get(url);
    if (!warmup) {
        state.SkipWithError("loopback warmup request failed");
        return;
    }

    for (auto _ : state) {
        auto result = client.get(url);
        if (!result) {
            state.SkipWithError("loopback request failed");
            break;
        }
        benchmark::DoNotOptimize(result.value().status_code());
        benchmark::DoNotOptimize(result.value().body().data());
    }

    state.SetBytesProcessed(
        state.iterations() * static_cast<std::int64_t>(body_size));
}

BENCHMARK(BM_LoopbackClient)
    ->Arg(32)
    ->Arg(4 * 1024)
    ->UseRealTime();

} // namespace
