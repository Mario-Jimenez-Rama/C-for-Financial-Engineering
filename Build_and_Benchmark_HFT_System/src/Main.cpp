#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
#include <cmath>

#include "../include/MarketData.hpp"
#include "../include/Order.hpp"
#include "../include/OrderBook.hpp"
#include "../include/OrderManager.hpp"
#include "../include/MatchingEngine.hpp"
#include "../include/Timer.hpp"
#include "../include/TradeLogger.hpp"

// Alias types used throughout the run
using Price  = double;
using OrderId= int;

using OrderType  = Order<Price, OrderId>;
using Book       = OrderBook<Price, OrderId>;
using OMS        = OrderManager<Price, OrderId>;
using Engine     = MatchingEngine<Price, OrderId>;
using TradeType  = Trade<Price, OrderId>;

static void analyzeLatencies(std::vector<long long>& latencies) {
    if (latencies.empty()) return;

    std::sort(latencies.begin(), latencies.end());
    long long minv = latencies.front();
    long long maxv = latencies.back();
    double mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();

    double variance = 0.0;
    for (auto l : latencies) {
        double d = (double)l - mean;
        variance += d * d;
    }
    double stddev = std::sqrt(variance / latencies.size());

    std::size_t p99_idx = static_cast<std::size_t>(latencies.size() * 0.99);
    if (p99_idx >= latencies.size()) p99_idx = latencies.size() - 1;
    long long p99 = latencies[p99_idx];

    std::cout << "Tick-to-Trade Latency (nanoseconds):\n";
    std::cout << "Min: " << minv
              << "\nMax: " << maxv
              << "\nMean: " << mean
              << "\nStdDev: " << stddev
              << "\nP99: " << p99 << "\n";
}

int main() {
    // --- Modules ------------------------------------------------------------
    Book   book;
    OMS    oms;
    Engine engine(book, oms);

    // Reserve for fewer rehashes under load
    constexpr std::size_t N_ORDERS = 100000;
    book.reserve(N_ORDERS);
    oms.reserve(N_ORDERS);

    // Trade logger: batch to CSV
    TradeLogger<TradeType> logger("trades.csv", 4096);

    // --- Generate mock market data -----------------------------------------
    std::vector<MarketData> ticks;
    MarketDataFeed feed(ticks);
    const int NUM_TICKS = 10000;
    feed.generateData(NUM_TICKS);

    // --- Create orders and measure tick-to-trade latency --------------------
    std::vector<long long> latencies;
    latencies.reserve(NUM_TICKS);

    // Randomize some aggressiveness so we get trades
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> side_dist(0,1);
    std::uniform_int_distribution<int> qty_dist(10, 200);
    std::uniform_real_distribution<Price> skew(-0.10, 0.10); // +/- 10 cents

    OrderId next_id = 1;

    for (int i = 0; i < NUM_TICKS; ++i) {
        const auto& md = ticks[i];

        // Simple strategy: place orders near mid with small skew to create crosses
        Price mid = (md.bid_price + md.ask_price) * 0.5;
        bool is_buy = (side_dist(rng) == 1);
        int qty = qty_dist(rng);
        Price px = mid + (is_buy ? +std::abs(skew(rng)) : -std::abs(skew(rng)));

        // Start latency timer at "tick received"
        Timer t; t.start();

        // Submit the order (engine will match immediately if it crosses)
        OrderType o{next_id++, px, qty, is_buy};
        auto trades = engine.submit(o);

        // If a trade was produced, stop the clock (tick -> trade)
        if (!trades.empty()) {
            long long ns = t.stop();
            latencies.push_back(ns);
            logger.append(trades); // batch write
        }
    }

    // Flush any buffered trades
    logger.flush();

    // Analyze latency
    analyzeLatencies(latencies);

    // Show top-of-book snapshot (optional)
    std::cout << "BestBid: " << book.bestBid() << "  BestAsk: " << book.bestAsk() << "\n";
    return 0;
}
