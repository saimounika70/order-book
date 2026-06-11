#pragma once
#include "order.h"
#include <array>
#include <deque>
#include <unordered_map>
#include <vector>

// Price is stored as integer ticks to avoid float indexing
// tick_size = 0.01, so price 100.50 → tick 10050
// MAX_PRICE_TICKS covers prices from 0 to 1000.00 in 0.01 increments

static constexpr double TICK_SIZE = 0.01;
static constexpr int MAX_PRICE_TICKS = 100'000; // $0 to $1000

inline int price_to_tick(double price)
{
    return static_cast<int>(price / TICK_SIZE + 0.5);
}
inline double tick_to_price(int tick)
{
    return tick * TICK_SIZE;
}

struct PriceLevel
{
    std::deque<Order> orders; // FIFO queue at this price level
    uint32_t total_qty = 0;   // cached sum — O(1) depth query

    bool empty() const { return orders.empty(); }
};

class OrderBookFast
{
public:
    OrderBookFast() : bids_(MAX_PRICE_TICKS), asks_(MAX_PRICE_TICKS) {}

    std::vector<Trade> add_order(Order order);
    bool cancel_order(uint64_t order_id);
    double best_bid() const;
    double best_ask() const;
    void print_top(int levels = 5) const;

private:
    std::vector<PriceLevel> bids_;
    std::vector<PriceLevel> asks_;

    int best_bid_tick_ = -1;
    int best_ask_tick_ = MAX_PRICE_TICKS;

    std::unordered_map<uint64_t, std::pair<Side, int>> order_index_;

    std::vector<Trade> match();
    void update_best_bid();
    void update_best_ask();
};