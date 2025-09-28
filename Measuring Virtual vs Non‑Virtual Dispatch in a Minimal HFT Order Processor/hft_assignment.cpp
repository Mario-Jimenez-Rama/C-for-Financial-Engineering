#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include <string>
#include <map>

// 1. PROBLEM SPECIFICATION

// Order struct with the exact specified layout
struct Order {
    uint64_t id;
    int side;      // 0 or 1
    int qty;
    int price;
    int payload[2];
};

// Function to generate N random orders with a deterministic seed for reproducibility
std::vector<Order> generate_random_orders(size_t n) {
    constexpr unsigned int seed = 12345;
    std::mt19937 rng(seed);

    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> qty_dist(1, 1000);
    std::uniform_int_distribution<int> price_dist(9900, 10100);
    std::uniform_int_distribution<int> payload_dist(0, 5000);

    std::vector<Order> orders;
    orders.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        orders.push_back(Order{
            .id = i,
            .side = side_dist(rng),
            .qty = qty_dist(rng),
            .price = price_dist(rng),
            .payload = {payload_dist(rng), payload_dist(rng)}
        });
    }
    
    return orders;
}

// 2. VIRTUAL IMPLEMENTATION

// Abstract base class
class Processor {
public:
    virtual uint64_t process(const Order& order) = 0;
    virtual ~Processor() = default;
};

// Derived class Strategy A (Virtual)
class StrategyA_V : public Processor {
public:
    uint64_t process(const Order& order) override {
        // Use static arrays to simulate a persistent order book in L1 cache
        static uint64_t book_prices[64] = {0};
        static int book_quantities[64] = {0};
        static uint64_t side_counter[2] = {0};

        // 6-10 integer arithmetic operations
        uint64_t checksum = order.id * 7;
        checksum += order.price;
        checksum -= order.qty;
        checksum ^= (order.price << 3);
        checksum += order.payload[0];
        checksum ^= order.payload[1];

        // Two small, fixed-size memory writes
        int index = order.id % 64;
        book_prices[index] = order.price;
        book_quantities[index] = order.qty;

        // One small conditional branch
        if (order.side == 0) {
            side_counter[0]++;
        } else {
            side_counter[1]++;
        }
        
        return checksum;
    }
};

// Derived class Strategy B (Virtual)
class StrategyB_V : public Processor {
public:
    uint64_t process(const Order& order) override {
        static uint64_t book_prices[64] = {0};
        static int book_quantities[64] = {0};
        static uint64_t side_counter[2] = {0};

        // Different but comparable work
        uint64_t checksum = order.id * 11;
        checksum ^= order.price;
        checksum += order.qty;
        checksum -= (order.qty << 2);
        checksum ^= order.payload[1];
        checksum += order.payload[0];

        // Two small, fixed-size memory writes
        int index = (order.id + 32) % 64; // Different access pattern
        book_prices[index] = order.price + 1;
        book_quantities[index] = order.qty - 1;

        // One small conditional branch
        if (order.side == 1) {
            side_counter[1]++;
        } else {
            side_counter[0]++;
        }
        
        return checksum;
    }
};


// 3. NON-VIRTUAL IMPLEMENTATION

// Concrete class Strategy A (Non-Virtual) - NO INHERITANCE
class StrategyA_NV {
public:
    uint64_t run(const Order& order) {
        // The work here MUST be IDENTICAL to StrategyA_V::process
        static uint64_t book_prices[64] = {0};
        static int book_quantities[64] = {0};
        static uint64_t side_counter[2] = {0};

        uint64_t checksum = order.id * 7;
        checksum += order.price;
        checksum -= order.qty;
        checksum ^= (order.price << 3);
        checksum += order.payload[0];
        checksum ^= order.payload[1];

        int index = order.id % 64;
        book_prices[index] = order.price;
        book_quantities[index] = order.qty;

        if (order.side == 0) {
            side_counter[0]++;
        } else {
            side_counter[1]++;
        }
        
        return checksum;
    }
};

// Concrete class Strategy B (Non-Virtual) - NO INHERITANCE
class StrategyB_NV {
public:
    uint64_t run(const Order& order) {
        // The work here MUST be IDENTICAL to StrategyB_V::process
        static uint64_t book_prices[64] = {0};
        static int book_quantities[64] = {0};
        static uint64_t side_counter[2] = {0};

        uint64_t checksum = order.id * 11;
        checksum ^= order.price;
        checksum += order.qty;
        checksum -= (order.qty << 2);
        checksum ^= order.payload[1];
        checksum += order.payload[0];

        int index = (order.id + 32) % 64;
        book_prices[index] = order.price + 1;
        book_quantities[index] = order.qty - 1;

        if (order.side == 1) {
            side_counter[1]++;
        } else {
            side_counter[0]++;
        }
        
        return checksum;
    }
};


// 4. MEASUREMENT HARNESS

int main() {
    // Setup
    const size_t WARMUP_N = 1000000;
    const size_t N = 2000000; // Chosen to yield ~0.5-2 sec runs
    const int REPEATS = 10;

    std::vector<Order> orders = generate_random_orders(N);

    // Create instances of all strategies
    StrategyA_V strategyA_v;
    StrategyB_V strategyB_v;
    StrategyA_NV strategyA_nv;
    StrategyB_NV strategyB_nv;

    // Define stream patterns
    std::map<std::string, std::vector<int>> patterns;
    patterns["homogeneous"] = std::vector<int>(N, 0); // All 'A'
    
    patterns["mixed_random"] = std::vector<int>();
    patterns["mixed_random"].reserve(N);
    std::mt19937 assign_rng(54321);
    std::uniform_int_distribution<int> assign_dist(0, 1);
    for (size_t i = 0; i < N; ++i) {
        patterns["mixed_random"].push_back(assign_dist(assign_rng));
    }

    patterns["bursty"] = std::vector<int>();
    patterns["bursty"].reserve(N);
    for (size_t i = 0; i < N; ++i) {
        patterns["bursty"].push_back((i % 80) < 64 ? 0 : 1); // 64 'A's, then 16 'B's
    }
    
    // An array of processor pointers for the virtual dispatch test
    std::vector<Processor*> virtual_processors(N);

    // Warmup
    std::cerr << "Warming up..." << std::endl; 
    volatile uint64_t warmup_checksum = 0;
    for (size_t i = 0; i < WARMUP_N; ++i) {
        warmup_checksum += strategyA_nv.run(orders[i % N]);
    }
    std::cerr << "Warmup complete. Checksum: " << warmup_checksum << std::endl; // Change to cerr

    // Print CSV header
    std::cout << "\npattern,impl,repeat,orders,elapsed_ns,ops_per_sec,checksum" << std::endl;

    // Main benchmark loop
    for (const auto& pair : patterns) {
        const std::string& pattern_name = pair.first;
        const std::vector<int>& assignments = pair.second;

        // Populate the virtual processors vector for this pattern
        for (size_t i = 0; i < N; ++i) {
            virtual_processors[i] = (assignments[i] == 0) ? (Processor*)&strategyA_v : (Processor*)&strategyB_v;
        }

        // VIRTUAL IMPLEMENTATION RUNS
        for (int r = 0; r < REPEATS; ++r) {
            volatile uint64_t total_checksum = 0; // The volatile sink
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < N; ++i) {
                total_checksum += virtual_processors[i]->process(orders[i]);
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            double ops_per_sec = (double)N / (elapsed_ns / 1e9);
            
            std::cout << pattern_name << ",virtual," << r << "," << N << "," << elapsed_ns << "," << ops_per_sec << "," << total_checksum << std::endl;
        }

        // NON-VIRTUAL IMPLEMENTATION RUNS
        for (int r = 0; r < REPEATS; ++r) {
            volatile uint64_t total_checksum = 0; // The volatile sink
            auto start = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < N; ++i) {
                if (assignments[i] == 0) {
                    total_checksum += strategyA_nv.run(orders[i]);
                } else {
                    total_checksum += strategyB_nv.run(orders[i]);
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            double ops_per_sec = (double)N / (elapsed_ns / 1e9);

            std::cout << pattern_name << ",non-virtual," << r << "," << N << "," << elapsed_ns << "," << ops_per_sec << "," << total_checksum << std::endl;
        }
    }

    return 0;
}