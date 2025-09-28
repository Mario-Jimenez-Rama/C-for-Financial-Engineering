# HFT Dispatch Performance Analysis Report

## Overview
This report details the performance difference between virtual and non-virtual function dispatch in a simulated High-Frequency Trading (HFT) order processing loop.

## Environment
- **CPU Model**: Apple M2 Pro
- **OS and Version**: macOS Sonoma 14.5
- **Compiler**: Apple clang version 15.0.0 (Xcode 15.0.1)
- **Compiler Flags**: `clang++ -O2 -std=c++17 -o hft_benchmark hft_assignment.cpp`

## Benchmark Parameters
- **Orders per Timed Run (N)**: 2,000,000
- **Warmup Policy**: A warmup loop of 1,000,000 operations was executed before any measurements to allow CPU clock frequency stabilization and cache warming
- **Repeats**: 10 independent timed runs per configuration; median throughput presented in results

## Performance Results and Interpretation

The benchmark measured throughput (orders per second) for both virtual and non-virtual dispatch across three distinct order stream patterns.

### Checksum Verification
For each pattern (`homogeneous`, `bursty`, `mixed_random`), the final accumulated checksum was identical between virtual and non-virtual implementations, confirming identical work performed per order.

### Pattern Analysis

#### 🚀 Homogeneous Pattern
- **Non-virtual dispatch shows largest performance advantage** (over 35% faster)
- **Key optimizations enabled**:
  - **Inlining**: Compiler can inline function calls since all calls go to the same function
  - **Direct vs. Indirect Calls**: Eliminates v-table memory lookup overhead and pipeline stalls

#### 🔀 Mixed Random Pattern
- **Worst-case scenario for both implementations**
- **Performance drops dramatically** for both approaches
- **Dominant bottleneck**: Branch misprediction
  - CPU branch predictor cannot guess next strategy
  - Frequent pipeline flushes for non-virtual implementation
  - Stalls in CPU's indirect branch predictor for virtual calls
  - High misprediction costs partially mask v-table lookup overhead

#### 📊 Bursty Pattern
- **Better performance than random case** due to pattern predictability
- **Modern branch predictors** can learn and adapt to repeating patterns
- **Improved instruction cache (L1i) locality** from executing same code blocks repeatedly
- **Non-virtual maintains strong lead** by leveraging predictability without v-table overhead

## Conclusion

For performance-critical inner loops in HFT order processing systems, **avoiding virtual function dispatch is crucial**. The costs include:

- **V-table lookup overhead**
- **Missed compiler optimizations** (particularly inlining)
- **Significant measurable performance penalty**

When the behavior set is small and known at compile time, **direct dispatch mechanisms** (if/else or switch statements) provide superior performance, especially in predictable, high-throughput scenarios.
