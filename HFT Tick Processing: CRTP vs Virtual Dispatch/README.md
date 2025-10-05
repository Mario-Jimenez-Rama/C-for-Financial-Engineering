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
