

#pragma once
#include <map>
#include <unordered_map>
#include <queue>
#include <vector>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <algorithm>
#include "Order.hpp"

/// Simple struct for each price level.
/// Keeps total quantity and count of active orders at this price.
struct alignas(64) PriceLevel {
    int totalQty = 0;
    int orderCount = 0;
};

/// Generic limit order book.
/// - Template on price and order ID type.
/// - Stores active price levels in std::map for ordering.
/// - Keeps ID → price lookup in unordered_map for O(1) amend/delete.
/// - Maintains lazy heaps for fast best bid/ask queries.
/// - Uses pre-reserved containers for performance.
template <typename PriceType, typename OrderIdType>
class OrderBook {
    static_assert(std::is_integral<OrderIdType>::value,
                  "OrderIdType must be integral");

public:
    using OrderT = Order<PriceType, OrderIdType>; // type alias for convenience

    // --- Core API -----------------------------------------------------------

    void newOrder(const OrderT& o) {
        auto& lvl = levels_[o.price]; //If the price doesn’t exist yet, the std::map automatically creates a new entry.
        lvl.totalQty   += o.quantity;
        lvl.orderCount += 1;
        id2price_[o.id] = o.price;

        if (o.is_buy) bidHeap_.push(o.price);
        else          askHeap_.push(o.price);
    }

    void amendOrder(OrderIdType id, int newQty) {
        auto it = id2price_.find(id);
        if (it == id2price_.end()) return; // unknown order

        PriceType px = it->second; // how to get price from order ID form an unordered_map
        auto lvlIt = levels_.find(px);
        if (lvlIt == levels_.end()) return;

        auto& lvl = lvlIt->second; // get the price level
        int diff = newQty - getOrderQty(id); // naive placeholder
        lvl.totalQty += diff;
        // you'd also update stored order quantity if you kept per-order detail
        id2qty_[id] = newQty;
    }

    void deleteOrder(OrderIdType id) {
        auto it = id2price_.find(id);
        if (it == id2price_.end()) return;

        PriceType px = it->second;
        auto lvlIt = levels_.find(px);
        if (lvlIt != levels_.end()) {
            auto& lvl = lvlIt->second; // get the price level
            int q = getOrderQty(id);
            lvl.totalQty   -= q;
            lvl.orderCount -= 1;
            if (lvl.orderCount <= 0) {
                levels_.erase(lvlIt);
            }
        }
        id2price_.erase(it);
        id2qty_.erase(id);
    }

    // --- Queries ------------------------------------------------------------

    /// Return best bid (max price with active orders)
    PriceType bestBid() {
        while (!bidHeap_.empty()) {
            auto p = bidHeap_.top();
            auto it = levels_.find(p); // gives you quantity and order count
            if (it != levels_.end() && it->second.orderCount > 0)
                return p;
            bidHeap_.pop(); // lazy eviction
        }
        return PriceType{}; // no bids
    }

    /// Return best ask (min price with active orders)
    PriceType bestAsk() {
        while (!askHeap_.empty()) {
            auto p = askHeap_.top();
            auto it = levels_.find(p);
            if (it != levels_.end() && it->second.orderCount > 0)
                return p;
            askHeap_.pop(); // lazy eviction
        }
        return PriceType{}; // no asks
    }

    size_t orderCount(PriceType px) const {
        auto it = levels_.find(px);
        return (it != levels_.end()) ? it->second.orderCount : 0;
    }

    int totalVolume(PriceType px) const {
        auto it = levels_.find(px);
        return (it != levels_.end()) ? it->second.totalQty : 0;
    }

    size_t levelCount() const noexcept { return levels_.size(); } // getter function, const to not change anytghing and noexcept no exceptions thrown

    // --- Preallocation / tuning --------------------------------------------
    void reserve(size_t maxOrders) {
        id2price_.reserve(maxOrders);
        id2qty_.reserve(maxOrders);
    }

private:
    // Helper: naive stub — could link to actual Order objects in OMS
    int getOrderQty(OrderIdType id) const {
        auto it = id2qty_.find(id);
        return (it != id2qty_.end()) ? it->second : 0;
    }

    // --- Data members -------------------------------------------------------
    std::map<PriceType, PriceLevel> levels_;             // active price levels
    std::unordered_map<OrderIdType, PriceType> id2price_; // order → price
    std::unordered_map<OrderIdType, int> id2qty_;         // order → quantity

    // Heaps for top-of-book
    std::priority_queue<PriceType> bidHeap_; // max-heap
    std::priority_queue<PriceType,
                        std::vector<PriceType>,
                        std::greater<PriceType>> askHeap_; // min-heap
};

