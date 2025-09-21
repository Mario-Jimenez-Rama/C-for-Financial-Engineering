#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cmath>

struct alignas(64) MarketData {
    int instrument_id; // instruments assigned by an integer id for each instance, not for each type
    double price;
    std::chrono::high_resolution_clock::time_point timestamp;
};

struct alignas(64) Order {
    int instrument_id;
    double price;
    bool is_buy;
    std::chrono::high_resolution_clock::time_point timestamp;
};

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
            md.instrument_id = i % 10;
            md.price = price_dist(gen);
            md.timestamp = std::chrono::high_resolution_clock::now();
            data.push_back(md);
        }
    }

private:
    std::vector<MarketData>& data;
};

class TradeEngine {
public:
    TradeEngine(const std::vector<MarketData>& feed)
        : market_data(feed) {}

    void process() {
        for (const auto& tick : market_data) {
            updateHistory(tick);
            bool buy = false, sell = false;

            if (signal1(tick)) { buy = true; counter1++; }
            if (signal2(tick)) {
                counter2++;
                if (tick.price < getAvg(tick.instrument_id)) {
                    buy = true;
                } else {
                    sell = true;
                }
            }
            if (signal3(tick)) { buy = true; counter3++; }
            if (signal4(tick)) { buy = true; counter4++; } 

            if (buy || sell) {
                auto now = std::chrono::high_resolution_clock::now();
                Order o{ tick.instrument_id, tick.price + (buy ? 0.01 : -0.01), buy, now };
                orders.push_back(o);
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(now - tick.timestamp).count();
                latencies.push_back(latency);
            }
        }
    }

    void exportOrderHistoryToCSV(const std::string& filename) {
        std::ofstream file(filename);
        file << "instrument_id,price,side,timestamp_ns\n";
        for (const auto& order : orders) {
            auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                order.timestamp.time_since_epoch()).count();
            file << order.instrument_id << ","
                 << order.price << ","
                 << (order.is_buy ? "BUY" : "SELL") << ","
                 << timestamp_ns << "\n";
        }
        std::cout << "Order history exported to " << filename << std::endl;
    }

    void visualizePrices(int instrument_id, const std::string& filename) {
        std::ofstream file(filename);
        file << "timestamp_ns,price\n";
        
        auto it = price_history.find(instrument_id);
        if (it != price_history.end()) {
            const auto& prices = it->second;
            for (size_t i = 0; i < prices.size(); ++i) {
                // Simulate timestamps for visualization
                auto fake_timestamp = std::chrono::nanoseconds(i * 1000000);
                file << fake_timestamp.count() << "," << prices[i] << "\n";
            }
        }
        std::cout << "Price visualization data for instrument " << instrument_id 
                  << " exported to " << filename << std::endl;
    }

    void reportStats() {
        long long sum = 0, max_latency = 0;
        for (auto l : latencies) {
            sum += l;
            if (l > max_latency) max_latency = l;
        }

        std::cout << "\n--- Performance Report ---\n";
        std::cout << "Total Market Ticks Processed: " << market_data.size() << "\n";
        std::cout << "Total Orders Placed: " << orders.size() << "\n";
        std::cout << "Average Tick-to-Trade Latency (ns): " << (latencies.empty() ? 0 : sum / latencies.size()) << "\n";
        std::cout << "Maximum Tick-to-Trade Latency (ns): " << max_latency << "\n";
        std::cout << "Signal 1 triggered: " << counter1 << " times\n";
        std::cout << "Signal 2 triggered: " << counter2 << " times\n";
        std::cout << "Signal 3 triggered: " << counter3 << " times\n";
        std::cout << "Signal 4 triggered: " << counter4 << " times\n";
    }

private:
    const std::vector<MarketData>& market_data;
    std::vector<Order> orders;
    std::vector<long long> latencies;
    std::unordered_map<int, std::vector<double>> price_history; // dictionary of price history (vector) per instrument (single int)
    int counter1 = 0, counter2 = 0, counter3 = 0, counter4 = 0;

    void updateHistory(const MarketData& tick) { // cannot preallocate as we don't know which instruments will appear
        auto& hist = price_history[tick.instrument_id]; // keep last 10 prices of each instrument, reference to avoid copies
        hist.push_back(tick.price);
        if (hist.size() > 10) hist.erase(hist.begin());
    }

    double getAvg(int id) {
        auto& hist = price_history[id];
        double sum = 0;
        for (double p : hist) sum += p;
        return hist.empty() ? 0 : sum / hist.size();
    }

    double getVolatility(int id) {
        auto& hist = price_history[id];
        if (hist.size() < 2) return 0.0;
        
        double mean = getAvg(id);
        double variance = 0.0;
        for (double price : hist) {
            variance += (price - mean) * (price - mean);
        }
        variance /= hist.size();
        return std::sqrt(variance);
    }

    bool signal1(const MarketData& tick) {
        return tick.price < 105.0 || tick.price > 195.0;
    }

    bool signal2(const MarketData& tick) {
        if (price_history[tick.instrument_id].size() < 5) return false;
        double avg = getAvg(tick.instrument_id);
        return tick.price < avg * 0.98 || tick.price > avg * 1.02;
    }

    bool signal3(const MarketData& tick) {
        auto& hist = price_history[tick.instrument_id];
        if (hist.size() < 3) return false;
        double diff1 = hist[hist.size() - 2] - hist[hist.size() - 3];
        double diff2 = hist[hist.size() - 1] - hist[hist.size() - 2];
        return diff1 > 0 && diff2 > 0;
    }

    bool signal4(const MarketData& tick) {
        if (price_history[tick.instrument_id].size() < 5) return false;
        
        double volatility = getVolatility(tick.instrument_id);
        double avg = getAvg(tick.instrument_id);
        double volatility_threshold = avg * 0.02; // 2% of average price
        
        // Buy if price is below average and volatility is high
        return (tick.price < avg * 0.99) && (volatility > volatility_threshold);
    }
};

int main() {
    std::vector<MarketData> feed;
    MarketDataFeed generator(feed);

    auto start = std::chrono::high_resolution_clock::now();
    generator.generateData(1000000);

    TradeEngine engine(feed);
    engine.process();

    auto end = std::chrono::high_resolution_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Export order history to CSV
    engine.exportOrderHistoryToCSV("order_history.csv");
    
    // Export price data for visualization (instrument 0 as example)
    engine.visualizePrices(0, "price_data_instrument_0.csv");
    
    // Report statistics
    engine.reportStats();
    std::cout << "Total Runtime (ms): " << runtime << std::endl;

    return 0;
}