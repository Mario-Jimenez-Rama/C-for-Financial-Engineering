#include <iostream>
#include <vector>
#include <chrono>
#include <random>


constexpr std::size_t kAlign = 64; // set to 1 for “off”
struct alignas(kAlign) MarketData {
    std::string symbol;
    double bid_price;
    double ask_price;
    std::chrono::high_resolution_clock::time_point timestamp;
};

// struct MarketData {
//     std::string symbol;
//     double bid_price;
//     double ask_price;
//     std::chrono::high_resolution_clock::time_point timestamp;
// };

class MarketDataFeed {
public:
    MarketDataFeed(std::vector<MarketData>& ref) : data(ref) {}

    void generateData(int num_ticks) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> price_dist(100.0, 200.0);

        data.reserve(num_ticks); // preallocate memory contiguously
        for (int i = 0; i < num_ticks; ++i) {
            MarketData md;
            md.symbol = "SYM" + std::to_string(i % 10);
            md.bid_price = price_dist(gen);
            md.ask_price = price_dist(gen) + 0.05; // small spread
            md.timestamp = std::chrono::high_resolution_clock::now();
            data.push_back(md);
        }
    }

private:
    std::vector<MarketData>& data;
};