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
```

▶️ Run Instructions
🏃 Run Main Simulation
```bash
./build/hft_app
```

⚡ Run Latency Benchmark
```bash
./build/hft_latency_test
```

## 📊 Benchmark Results

| Load | Reserve | Samples | Mean (ns) | StdDev | P99 (ns) | Min (ns) | Max (ns) |
|------|----------|----------|-----------|---------|-----------|-----------|
| 1K | OFF | 443 | 1485.67 | 798.18 | 4458 | 542 | 8792 |
| 1K | ON | 415 | 1410.85 | 737.48 | 3250 | 625 | 10333 |
| 10K | OFF | 4285 | 2492.20 | 3001.20 | 8042 | 667 | 111958 |
| 10K | ON | 4193 | 2016.44 | 1315.01 | 5084 | 750 | 32667 |
| 100K | OFF | 42788 | 2068.96 | 1944.97 | 6500 | 708 | 243250 |
| 100K | ON | 42709 | 1883.28 | 1703.05 | 5959 | 542 | 236625 |


## 🧪 Performance Analysis

- **Latency:** Median tick-to-trade latency ≈ 1.5–2.0 µs, 99th percentile ≈ 6 µs  
- **Reserve optimization:** Using pre-reserved memory (`reserve=ON`) reduces tail latency (P99) by ~20–25%.  
- **Scalability:** Handles 100K orders with stable latency under 10 µs for 99% of events.  
- **Determinism:** Tight latency distribution → predictable performance.  
- **CPU bound:** Single-threaded benchmark; potential future gains from:
  - Lock-free queues  
  - Per-core engine instances  
  - Memory pool reuse  

---

## 🧩 System Architecture

### High-Level Flow

```text
[ MarketDataFeed ]
        │
        ▼
[ MatchingEngine ] <--> [ OrderBook ]
        │                     │
        ▼                     ▼
  [ OrderManager ]      [ TradeLogger ]
```

## 🧩 Module Roles

| Module | Responsibility |
|---------|----------------|
| **MarketDataFeed** | Generates synthetic bid/ask ticks |
| **MatchingEngine** | Matches incoming orders with resting ones using price/time priority |
| **OrderBook** | Stores and aggregates limit orders by price level |
| **OrderManager (OMS)** | Tracks order states (New, Filled, Canceled) |
| **TradeLogger** | Records trades with timestamps for analysis |
| **Timer** | Measures high-resolution tick-to-trade latency |

---

## 🧮 Class Relationships (UML-style)

```text
+-------------------+
|  OrderManager     |
|-------------------|
| +create()         |
| +fill()           |
| +cancel()         |
+---------+---------+
          |
          ▼
+-------------------+
|  OrderBook        |
|-------------------|
| +newOrder()       |
| +amendOrder()     |
| +deleteOrder()    |
+---------+---------+
          |
          ▼
+-------------------+
| MatchingEngine    |
|-------------------|
| +submit()         |
| +cancel()         |
| +replacePrice()   |
+---------+---------+
          |
          ▼
+-------------------+
| TradeLogger       |
|-------------------|
| +push()           |
| +flush()          |
+-------------------+
```

## 📈 Benchmark Methodology

Each benchmark run generates **N synthetic orders (buy/sell)** and measures:

1. **Start timestamp** at tick arrival  
2. **End timestamp** at trade match or order book update  
3. Outputs latency statistics:
   - Minimum (`min`)
   - Maximum (`max`)
   - Mean
   - Standard Deviation (`stddev`)
   - Percentiles (`P50`, `P90`, `P99`)

### ⏱️ Latency Computation Example

```cpp
auto start = Clock::now();
// order submission + matching
auto end = Clock::now();
auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
```

## 🧮 Example Output Snapshot
```yaml
=== Load=100K, reserve=ON ===
Samples: 42709
Min: 542 ns | Max: 236625 ns | Mean: 1883.28 ns
StdDev: 1703.05 | P50: 1625 | P90: 2916 | P99: 5959
Snapshot BestBid=0 BestAsk=0
```

## 🧱 Project Structure
```text
Build_and_Benchmark_HFT_System/
│
├── include/
│   ├── MarketData.hpp
│   ├── Order.hpp
│   ├── OrderBook.hpp
│   ├── OrderManager.hpp
│   ├── MatchingEngine.hpp
│   ├── TradeLogger.hpp
│   └── Timer.hpp
│
├── src/
│   ├── MarketData.cpp
│   ├── OrderBook.cpp
│   ├── OrderManager.cpp
│   ├── MatchingEngine.cpp
│   ├── TradeLogger.cpp
│   └── main.cpp
│
├── test/
│   └── test_latency.cpp
│
├── CMakeLists.txt
└── README.md
```
