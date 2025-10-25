#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "../include/MarketData.hpp"
#include "../include/Order.hpp"
#include "../include/OrderBook.hpp"
#include "../include/OrderManager.hpp"
#include "../include/MatchingEngine.hpp"
#include "../include/Timer.hpp"
#include "../include/TradeLogger.hpp"

// Type aliases for convenience
using Price   = double;
using OrderId = int;

using OrderType = Order<Price, OrderId>;
using Book      = OrderBook<Price, OrderId>;
using OMS       = OrderManager<Price, OrderId>;
using Engine    = MatchingEngine<Price, OrderId>;
using TradeType = Trade<Price, OrderId>;

struct Stats {
    long long minv{};
    long long maxv{};
    double mean{};
    double stddev{};
    long long p50{};
    long long p90{};
    long long p99{};
    std::size_t samples{};
};

static Stats compute_stats(std::vector<long long>& lat) {
    Stats s{};
    if (lat.empty()) return s;
    std::sort(lat.begin(), lat.end());
    s.samples = lat.size();
    s.minv = lat.front();
    s.maxv = lat.back();
    s.mean = std::accumulate(lat.begin(), lat.end(), 0.0) / lat.size();
    double var = 0.0;
    for (auto v : lat) {
        double d = (double)v - s.mean;
        var += d * d;
    }
    s.stddev = std::sqrt(var / lat.size());
    auto idx = [&](double q) {
        std::size_t i = static_cast<std::size_t>(q * (lat.size() - 1));
        if (i >= lat.size()) i = lat.size() - 1;
        return lat[i];
    };
    s.p50 = idx(0.50);
    s.p90 = idx(0.90);
    s.p99 = idx(0.99);
    return s;
}

static void print_stats(const std::string& title, const Stats& s) {
    std::cout << "=== " << title << " ===\n";
    if (s.samples == 0) {
        std::cout << "No samples.\n\n";
        return;
    }
    std::cout << "Samples: " << s.samples
              << "\nMin: "    << s.minv
              << "\nMax: "    << s.maxv
              << "\nMean: "   << s.mean
              << "\nStdDev: " << s.stddev
              << "\nP50: "    << s.p50
              << "\nP90: "    << s.p90
              << "\nP99: "    << s.p99
              << "\n\n";
}

struct TrialConfig {
    int num_ticks;
    bool pre_reserve;     // experiment: reserve() vs no reserve()
    bool write_trades;    // optionally write CSV (disabled by default to reduce I/O noise)
    std::string label;
};

static Stats run_trial(const TrialConfig& cfg) {
    // Modules
    Book   book;
    OMS    oms;
    Engine engine(book, oms);

    // Optional pre-reserve to reduce rehashing and vector growth
    if (cfg.pre_reserve) {
        std::size_t N = static_cast<std::size_t>(cfg.num_ticks);
        book.reserve(N);
        oms.reserve(N);
    }

    // Optional trade logger (batching). Keep off by default to avoid I/O impacting latency.
    std::unique_ptr<TradeLogger<TradeType>> logger;
    if (cfg.write_trades) {
        logger = std::make_unique<TradeLogger<TradeType>>("trades_" + cfg.label + ".csv", 4096);
    }

    // Generate ticks
    std::vector<MarketData> ticks;
    MarketDataFeed feed(ticks);
    feed.generateData(cfg.num_ticks);

    // Simple synthetic order flow around mid to provoke crosses
    std::mt19937 rng(2025);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> qty_dist(10, 200);
    std::uniform_real_distribution<Price> skew(0.0, 0.10); // 0..10 cents (abs, sign set by side)

    std::vector<long long> latencies;
    latencies.reserve(cfg.num_ticks);

    OrderId next_id = 1;

    for (int i = 0; i < cfg.num_ticks; ++i) {
        const auto& md = ticks[i];
        const Price mid = (md.bid_price + md.ask_price) * 0.5;

        const bool is_buy = (side_dist(rng) == 1);
        const int qty = qty_dist(rng);
        const Price px = is_buy ? (mid + skew(rng)) : (mid - skew(rng));

        Timer t; t.start();

        OrderType o{next_id++, px, qty, is_buy};
        auto trades = engine.submit(o);

        if (!trades.empty()) {
            long long ns = t.stop();
            latencies.push_back(ns);
            if (logger) logger->append(trades);
        }
    }

    if (logger) logger->flush();

    auto stats = compute_stats(latencies);
    print_stats(cfg.label, stats);
    return stats;
}

int main() {
    // Experiments per the exercise:
    // - Load scaling: 1K, 10K, 100K ticks
    // - Container preallocation: reserve() ON vs OFF
    // You can add more trials for alignas(64) toggle, allocators, container layout, etc.

    std::vector<TrialConfig> trials = {
        { 1'000,  false, false, "Load=1K, reserve=OFF" },
        { 1'000,  true,  false, "Load=1K, reserve=ON"  },
        { 10'000, false, false, "Load=10K, reserve=OFF" },
        { 10'000, true,  false, "Load=10K, reserve=ON"  },
        { 100'000,false, false, "Load=100K, reserve=OFF" },
        { 100'000,true,  false, "Load=100K, reserve=ON"  },
    };

    // Run all trials
    for (const auto& cfg : trials) {
        run_trial(cfg);
    }

    // Optional: show current top-of-book state in a small sanity check run
    {
        Book book;
        OMS  oms;
        Engine engine(book, oms);
        book.reserve(10000);
        oms.reserve(10000);

        // Simple two-order cross
        engine.submit(OrderType{1, 100.5, 100, true});  // buy
        engine.submit(OrderType{2, 100.4, 100, false}); // sell

        std::cout << "Snapshot BestBid=" << book.bestBid()
                  << " BestAsk=" << book.bestAsk() << "\n";
    }

    return 0;
}
