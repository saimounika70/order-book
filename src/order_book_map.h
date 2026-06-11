// BASELINE (slow, but correct)
// We use this to verify correctness before optimising.
#pragma once
#include "order.h"
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <functional>

class OrderBookMap
{
public:
    // add a new limit order, returns any trades that resulted
    std::vector<Trade> add_order(Order order);

    // cancel an existing order by id
    bool cancel_order(uint64_t order_id);

    double best_bid() const;
    double best_ask() const;
    void print_top(int levels = 5) const;

private:
    // bids: highest price first → use greater<double>
    std::map<double, std::list<Order>, std::greater<double>> bids_;
    // asks: lowest price first → default less<double>
    std::map<double, std::list<Order>> asks_;

    // id → iterator into the map, for O(1) cancel
    std::unordered_map<uint64_t,
                       std::pair<bool, std::list<Order>::iterator>>
        order_index_;

    std::vector<Trade> match();
    uint64_t next_trade_id_ = 1;
};