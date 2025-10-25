# Build & Benchmark HFT System (C++17)

## 🎯 Objective
Design and benchmark a simplified high-frequency trading (HFT) prototype using modern C++ features:
- RAII and smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Template-based OrderBook and MatchingEngine
- Custom alignment and preallocation for cache efficiency
- Tick-to-trade latency benchmarking

## 🧩 Modules
| Module | Description |
|---------|-------------|
| **MarketDataFeed** | Simulates market ticks with alignas(64) for cache optimization |
| **OrderManager (OMS)** | Manages order lifecycle (new, fill, cancel) with shared_ptr |
| **OrderBook** | Stores active price levels and aggregates volumes |
| **MatchingEngine** | Matches buy/sell orders in price-time priority and returns trades |
| **TradeLogger** | Batches and logs trades safely with RAII |
| **Timer** | Measures nanosecond-level latency |
| **Test Harness** | Benchmarks tick-to-trade latency under load |

## 🏗️ Build
```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
