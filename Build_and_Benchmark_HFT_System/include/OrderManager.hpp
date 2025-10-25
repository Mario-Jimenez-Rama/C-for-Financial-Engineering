#pragma once
#include <unordered_map>
#include <memory>
#include <type_traits>
#include "Order.hpp"

enum class OrderState : unsigned char {
    New,
    PartiallyFilled,
    Filled,
    Canceled
};

template <typename PriceType, typename OrderIdType>
class OrderManager {
    static_assert(std::is_integral<OrderIdType>::value,
                  "OrderIdType must be integral");

public:
    using OrderT      = Order<PriceType, OrderIdType>;
    using OrderHandle = std::shared_ptr<OrderT>;

    // Optionally pre-reserve to avoid rehashing under load
    void reserve(std::size_t n) {
        orders_.reserve(n);
        states_.reserve(n);
    }

    // --- Create / Cancel / Fill -------------------------------------------

    // The [[nodiscard]] attribute warns if the return value is ignored
    [[nodiscard]] OrderHandle create(OrderIdType id, PriceType price, int qty, bool is_buy) {
        auto o = std::make_shared<OrderT>(id, price, qty, is_buy);
        orders_[id] = o;
        states_[id] = OrderState::New;
        return o;
    }

    bool cancel(OrderIdType id) {
        auto it = orders_.find(id);
        if (it == orders_.end()) return false;
        auto st = state(id);
        if (st == OrderState::Filled || st == OrderState::Canceled) return false;
        states_[id] = OrderState::Canceled;
        return true;
    }

    // Apply a trade fill to this order
    bool fill(OrderIdType id, int exec_qty) {
        if (exec_qty <= 0) return false;
        auto it = orders_.find(id);
        if (it == orders_.end()) return false;

        auto st = state(id);
        if (st == OrderState::Canceled || st == OrderState::Filled) return false;

        auto& o = *it->second; // * is t see what the order that the shared_ptr points to and & to get a reference to it
        // -avoid copying the whole Order struct, we are working with the actual order
        // -avoid increasing the reference count on the shared pointer,
        // -and allow direct modification of the actual order.
        if (exec_qty >= o.quantity) {
            o.quantity = 0;
            states_[id] = OrderState::Filled;
        } else {
            o.quantity -= exec_qty;
            states_[id] = OrderState::PartiallyFilled;
        }
        return true;
    }

    // --- Amend / Replace ---------------------------------------------------
    // Change remaining quantity (not side/price)
    bool amendQuantity(OrderIdType id, int new_qty) {
        if (new_qty < 0) return false;
        auto it = orders_.find(id);
        if (it == orders_.end()) return false;

        auto st = state(id);
        if (st == OrderState::Canceled || st == OrderState::Filled) return false;

        it->second->quantity = new_qty;
        // Update state based on new remaining
        states_[id] = (new_qty == 0) ? OrderState::Filled
                                     : (st == OrderState::New ? OrderState::New : OrderState::PartiallyFilled);
        return true;
    }

    // Change price (a "replace"); many venues treat as cancel+new in the book.
    bool replacePrice(OrderIdType id, PriceType new_price) {
        auto it = orders_.find(id);
        if (it == orders_.end()) return false;

        auto st = state(id);
        if (st == OrderState::Canceled || st == OrderState::Filled) return false;

        it->second->price = new_price;
        return true;
    }

    // --- Queries -----------------------------------------------------------
    [[nodiscard]] OrderState state(OrderIdType id) const {
        auto it = states_.find(id);
        return it == states_.end() ? OrderState::Canceled : it->second;
    }

    [[nodiscard]] OrderHandle get(OrderIdType id) const {
        auto it = orders_.find(id);
        return it == orders_.end() ? nullptr : it->second;
    }

    [[nodiscard]] bool exists(OrderIdType id) const {
        return orders_.find(id) != orders_.end();
    }

    [[nodiscard]] int getRemainingQty(OrderIdType id) const {
        auto it = orders_.find(id);
        return it == orders_.end() ? 0 : it->second->quantity;
    }

    [[nodiscard]] PriceType getPrice(OrderIdType id) const {
        auto it = orders_.find(id);
        return it == orders_.end() ? PriceType{} : it->second->price;
    }

    [[nodiscard]] bool isBuy(OrderIdType id) const {
        auto it = orders_.find(id);
        return it != orders_.end() && it->second->is_buy;
    }

private:
    std::unordered_map<OrderIdType, OrderHandle> orders_;
    std::unordered_map<OrderIdType, OrderState>  states_;
};
