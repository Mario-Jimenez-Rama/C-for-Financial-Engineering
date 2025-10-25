#pragma once
#include <type_traits>

// -----------------------------------------------------------------------------
// Generic Order structure
// -----------------------------------------------------------------------------
// Template parameters:
//   PriceType   -> usually double, but can be int (ticks) for performance tests
//   OrderIdType -> integer type (int, long, etc.)
// -----------------------------------------------------------------------------

template <typename PriceType, typename OrderIdType>
struct Order {
    static_assert(std::is_integral<OrderIdType>::value,
                  "OrderIdType must be an integral type");

    OrderIdType id;   // unique order ID
    PriceType price;  // order price (limit)
    int quantity;  // order size
    bool is_buy;    // true = buy, false = sell

    // Constructor for convenience (optional)
    Order(OrderIdType oid, PriceType px, int qty, bool buy)
        : id(oid), price(px), quantity(qty), is_buy(buy) {}

    // Default constructor
    Order() = default;
};


