# hft-crtp-assignment

A microbenchmark comparing **free function**, **runtime polymorphism (virtual)**, and **CRTP static polymorphism** for a simple HFT-style per-tick signal:

\[
\text{signal} = \alpha_1 (\text{microprice} - \text{mid}) + \alpha_2 \cdot \text{imbalance}
\]

## Project Structure

hft-crtp-assignment/
include/
market_data.hpp
utils.hpp
strategy_virtual.hpp
strategy_crtp.hpp
src/
main.cpp

## Build & Run 
```bash
# from project root
clang++ -std=c++20 -O3 -DNDEBUG -march=native -fno-exceptions -fno-rtti \
  -Iinclude src/main.cpp -o hft

# run: [N_TICKS] [ITERS]
./hft 10000000 1

```markdown
# Benchmark Results

**Setup**
- OS: macOS (Apple Silicon)
- Compiler: Apple Clang (C++20), flags: `-O3 -DNDEBUG -march=native -fno-exceptions -fno-rtti`
- Workload: 10,000,000 ticks, iters=1 (synthetic top-of-book stream, AoS layout)

**Raw output** 
free_function time: 36.805 ms sink=0.000000 
virtual_call time: 36.950 ms sink=0.000000 
crtp_call time: 35.505 ms sink=0.000000 

=== Summary === 
free_function ns/tick: 3.681 ticks/sec: 271.70 M 
virtual_call ns/tick: 3.695 ticks/sec: 270.64 M 
crtp_call ns/tick: 3.550 ticks/sec: 281.65 M

**Table**

| Variant           | Time (ms) | ns/tick | ticks/sec (M) |
|------------------|-----------:|--------:|--------------:|
| free_function    | 36.805     | 3.681   | 271.70        |
| virtual_call     | 36.950     | 3.695   | 270.64        |
| crtp_call        | 35.505     | 3.550   | 281.65        |

**Deltas**
- CRTP vs **virtual**: **~3.92% faster** ( (3.695 − 3.550) / 3.695 )
- CRTP vs **free**: **~3.56% faster** ( (3.681 − 3.550) / 3.681 )
- **virtual** vs free: ~0.38% slower

```markdown
# CRTP vs Virtual Dispatch for a Per-Tick Signal

## Problem
Compute on each tick:
\[
\text{signal} = \alpha_1 (\text{microprice} - \text{mid}) + \alpha_2 \cdot \text{imbalance}
\]
and compare three variants:
1) Free function (control), 2) Runtime polymorphism (virtual), 3) CRTP static polymorphism.

## Setup
- **OS/CPU**: Apple Silicon (macOS)
- **Compiler/flags**: Apple Clang, `-std=c++20 -O3 -DNDEBUG -march=native -fno-exceptions -fno-rtti`
- **Workload**: 10M ticks, iters=1 (deterministic XorShift32 generator). Functions are header-only to encourage inlining.
- **Layout**: Array-of-Structs (`std::vector<Quote>`).

## Results
| Variant           | ns/tick | ticks/sec (M) |
|-------------------|--------:|--------------:|
| Free function     | 3.681   | 271.70        |
| Virtual call      | 3.695   | 270.64        |
| **CRTP**          | **3.550** | **281.65**   |

**Observations**
- **CRTP ≈ Free**, and both outperform **Virtual** slightly.
- CRTP is ~**3.9%** faster than virtual, ~**3.6%** faster than free.
- Virtual ≈ free in this test (only **0.4%** slower), suggesting the indirect call overhead is small relative to the math (2 divides, several muls/adds).

## Why CRTP wins (here, modestly)
- **Inlining & constant propagation**: CRTP removes the virtual call boundary so `on_tick` and `on_tick_impl` fully inline. `alpha1/alpha2` can propagate, common subexpressions can be optimized.
- **No indirect branch**: avoids vtable load & indirect call; even though it’s well-predicted in a tight loop, removing it can still shave a few cycles and enable better scheduling.

## Why virtual is close to free
- The **floating-point arithmetic dominates** the tick cost; the virtual dispatch (one well-predicted indirect call) contributes little when the body is non-trivial.
- Modern branch predictors quickly lock onto a single target in a hot loop.

## When to use CRTP in HFT systems
- **Hot-path code you control at compile time**: market-data transforms, per-tick indicators, microstructure features.
- When you want **zero-overhead abstraction** and the ability to **inline across the interface**.
- When ABI stability and plugin boundaries are **not** required.

## When to keep virtual
- **Plugin or runtime selection** of behaviors; late binding across modules.
- Complex dispatch graphs where code-size and modularity outweigh a few percent of speed.
- Public API boundaries where ABI stability matters.

## Notes & Guidance
- **Counters**: On Linux, add `perf stat -e cycles,instructions,branches,branch-misses` to capture IPC and branch miss rate; CRTP usually has slightly higher IPC and lower miss rate.
- **Reproducibility**: pin the process (`taskset -c 0`), fix the RNG seed (already done), run several trials; report mean ± stdev.
- **Numerics**: if you test `-Ofast`/`-ffast-math`, document any deviations (denormals, reassociation).

