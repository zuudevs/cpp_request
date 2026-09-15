#include <benchmark/benchmark.h>

#include "loopback_benchmark_server.hpp"

#include <cpp_request/client.hpp>

#include <chrono>
#include <string>

namespace {

void configure(cpp_request::Client& client) {
    client.set_connect_timeout(std::chrono::seconds{2});
    client.set_read_timeout(std::chrono::seconds{2});
    client.set_write_timeout(std::chrono::seconds{2});
}

void BM_ConnectionReuse(benchmark::State& state) {
    cpp_request::bench::LoopbackBenchmarkServer server{32};
    if (!server.valid()) {
        state.SkipWithError("failed to create loopback server");
        return;
    }

    cpp_request::Client client;
    configure(client);
    const std::string url = server.url("/reuse");

    auto warmup = client.get(url);
    if (!warmup) {
        state.SkipWithError("keep-alive warmup request failed");
        return;
    }

    for (auto _ : state) {
        auto result = client.get(url);
        if (!result) {
            state.SkipWithError("keep-alive request failed");
            break;
        }
        benchmark::DoNotOptimize(result.value().body().data());
    }

    state.SetItemsProcessed(state.iterations());
}

void BM_ConnectionReconnect(benchmark::State& state) {
    cpp_request::bench::LoopbackBenchmarkServer server{32};
    if (!server.valid()) {
        state.SkipWithError("failed to create loopback server");
        return;
    }

    const std::string url = server.url("/reconnect");

    for (auto _ : state) {
        cpp_request::Client client;
        configure(client);
        auto result = client.get(url);
        if (!result) {
            state.SkipWithError("reconnect request failed");
            break;
        }
        benchmark::DoNotOptimize(result.value().body().data());
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ConnectionReuse)->UseRealTime();
BENCHMARK(BM_ConnectionReconnect)->UseRealTime();

} // namespace
