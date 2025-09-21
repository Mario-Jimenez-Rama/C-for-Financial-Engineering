# High-Frequency Trading (HFT) System Simulator
A C++ simulation of a high-frequency trading engine that processes synthetic market data through algorithmic signals to generate orders, with a focus on performance benchmarking and data export.

## Overview
This project models the core components of a low-latency trading system. It generates a high-volume stream of simulated price data (ticks) for multiple financial instruments. A trade engine then analyzes each data tick in real-time, applying algorithmic signals to make automated buy/sell decisions. The system meticulously tracks performance metrics like processing latency and exports all results for analysis.

## Key Features
- Synthetic Market Data Generation: Creates a configurable volume of realistic price ticks for multiple instruments.

- Low-Latency Processing: Engineered with performance in mind, using cache-aligned data structures.

- Algorithmic Trading Signals: The core logic is driven by four distinct trading signals:

  - Signal 1 (Extreme Price Check): Triggers on absolute price outliers (< $105.00 or > $195.00).

  - Signal 2 (Mean Reversion): Triggers a buy if price is >2% below the simple moving average (SMA) or a sell if >2% above.

  - Signal 3 (Momentum): Triggers a buy on two consecutive positive price changes.

  - Signal 4 (Volatility & Mean Reversion): Triggers a buy only when the price is below the SMA and current volatility is high.

- Performance Analytics: Tracks and reports detailed statistics, including nanosecond-grade tick-to-trade latency.

- Data Export: Outputs order history and price data to CSV files.

- Data Visualization: The code VisualizePrices.py plot and visualize the results

This are the results for n=100000 Ticks:

--- Performance Report ---
Total Market Ticks Processed: 100000
Total Orders Placed: 95468
Average Tick-to-Trade Latency (ns): 65434330
Maximum Tick-to-Trade Latency (ns): 120529083
Signal 1 triggered: 10039 times
Signal 2 triggered: 93185 times
Signal 3 triggered: 16724 times
Signal 4 triggered: 48181 times
Total Runtime (ms): 130

This are the results for n=1000000 Ticks:
--- Performance Report ---
Total Market Ticks Processed: 1000000
Total Orders Placed: 955639
Average Tick-to-Trade Latency (ns): 726587156
Maximum Tick-to-Trade Latency (ns): 1353629250
Signal 1 triggered: 100065 times
Signal 2 triggered: 933217 times


Signal 3 triggered: 166640 times
Signal 4 triggered: 482983 times
Total Runtime (ms): 1449
