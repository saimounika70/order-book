#include "order_book_fast.h"
#include <chrono>
#include <iostream>
#include <iomanip>

static uint64_t now_ns_fast()
{
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

std::vector<Trade> OrderBookFast::add_order(Order order)
{
    order.timestamp = now_ns_fast();
    int tick = price_to_tick(order.price);

    if (order.side == Side::BUY)
    {
        bids_[tick].orders.push_back(order);
        bids_[tick].total_qty += order.quantity;
        if (tick > best_bid_tick_)
            best_bid_tick_ = tick;
        order_index_[order.id] = {Side::BUY, tick};
    }
    else
    {
        asks_[tick].orders.push_back(order);
        asks_[tick].total_qty += order.quantity;
        if (tick < best_ask_tick_)
            best_ask_tick_ = tick;
        order_index_[order.id] = {Side::SELL, tick};
    }

    return match();
}

std::vector<Trade> OrderBookFast::match()
{
    std::vector<Trade> trades;

    while (best_bid_tick_ >= best_ask_tick_ &&
           best_bid_tick_ >= 0 &&
           best_ask_tick_ < MAX_PRICE_TICKS)
    {

        PriceLevel &bid_level = bids_[best_bid_tick_];
        PriceLevel &ask_level = asks_[best_ask_tick_];

        if (bid_level.empty())
        {
            update_best_bid();
            continue;
        }
        if (ask_level.empty())
        {
            update_best_ask();
            continue;
        }

        Order &bid = bid_level.orders.front();
        Order &ask = ask_level.orders.front();

        uint32_t qty = std::min(bid.remaining(), ask.remaining());
        double price = tick_to_price(best_ask_tick_);

        bid.filled += qty;
        ask.filled += qty;
        bid_level.total_qty -= qty;
        ask_level.total_qty -= qty;

        trades.push_back({bid.id, ask.id, price, qty, now_ns_fast()});

        if (bid.is_filled())
        {
            order_index_.erase(bid.id);
            bid_level.orders.pop_front();
            if (bid_level.empty())
                update_best_bid();
        }
        if (ask.is_filled())
        {
            order_index_.erase(ask.id);
            ask_level.orders.pop_front();
            if (ask_level.empty())
                update_best_ask();
        }
    }
    return trades;
}

void OrderBookFast::update_best_bid()
{
    while (best_bid_tick_ >= 0 && bids_[best_bid_tick_].empty())
        --best_bid_tick_;
}
void OrderBookFast::update_best_ask()
{
    while (best_ask_tick_ < MAX_PRICE_TICKS && asks_[best_ask_tick_].empty())
        ++best_ask_tick_;
}

bool OrderBookFast::cancel_order(uint64_t id)
{
    auto it = order_index_.find(id);
    if (it == order_index_.end())
        return false;

    auto [side, tick] = it->second;
    PriceLevel &level = (side == Side::BUY) ? bids_[tick] : asks_[tick];

    for (auto oit = level.orders.begin(); oit != level.orders.end(); ++oit)
    {
        if (oit->id == id)
        {
            level.total_qty -= oit->remaining();
            level.orders.erase(oit);
            break;
        }
    }
    if (level.empty())
    {
        if (side == Side::BUY && tick == best_bid_tick_)
            update_best_bid();
        if (side == Side::SELL && tick == best_ask_tick_)
            update_best_ask();
    }
    order_index_.erase(it);
    return true;
}

double OrderBookFast::best_bid() const
{
    return best_bid_tick_ < 0 ? 0.0 : tick_to_price(best_bid_tick_);
}
double OrderBookFast::best_ask() const
{
    return best_ask_tick_ >= MAX_PRICE_TICKS ? 0.0 : tick_to_price(best_ask_tick_);
}

void OrderBookFast::print_top(int levels) const
{
    std::cout << "\n=== FAST ORDER BOOK ===\n";
    int shown = 0;
    // collect ask levels to print in reverse
    std::vector<std::pair<int, uint32_t>> ask_levels;
    for (int t = best_ask_tick_; t < MAX_PRICE_TICKS && shown < levels; ++t)
    {
        if (!asks_[t].empty())
        {
            ask_levels.push_back({t, asks_[t].total_qty});
            ++shown;
        }
    }
    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it)
        std::cout << std::setw(10) << it->second
                  << std::setw(12) << std::fixed
                  << std::setprecision(2) << tick_to_price(it->first) << "\n";
    std::cout << "  -------- spread --------\n";
    shown = 0;
    for (int t = best_bid_tick_; t >= 0 && shown < levels; --t)
    {
        if (!bids_[t].empty())
        {
            std::cout << std::setw(10) << bids_[t].total_qty
                      << std::setw(12) << std::fixed
                      << std::setprecision(2) << tick_to_price(t) << "\n";
            ++shown;
        }
    }
}