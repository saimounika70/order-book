#include "order_book_map.h"
#include <chrono>
#include <iostream>
#include <iomanip>

static uint64_t now_ns()
{
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

std::vector<Trade> OrderBookMap::add_order(Order order)
{
    order.timestamp = now_ns();

    if (order.side == Side::BUY)
        bids_[order.price].push_back(order);
    else
        asks_[order.price].push_back(order);

    // store iterator for fast cancel
    if (order.side == Side::BUY)
    {
        auto &lst = bids_[order.price];
        order_index_[order.id] = {true, std::prev(lst.end())};
    }
    else
    {
        auto &lst = asks_[order.price];
        order_index_[order.id] = {false, std::prev(lst.end())};
    }

    return match();
}

std::vector<Trade> OrderBookMap::match()
{
    std::vector<Trade> trades;

    while (!bids_.empty() && !asks_.empty())
    {
        auto &[bid_price, bid_list] = *bids_.begin();
        auto &[ask_price, ask_list] = *asks_.begin();

        // no match possible
        if (bid_price < ask_price)
            break;

        Order &bid = bid_list.front();
        Order &ask = ask_list.front();

        uint32_t qty = std::min(bid.remaining(), ask.remaining());
        double price = bid_price; //
        bid.filled += qty;
        ask.filled += qty;

        trades.push_back({bid.id, ask.id, price, qty, now_ns()});

        // remove fully filled orders
        if (bid.is_filled())
        {
            order_index_.erase(bid.id);
            bid_list.pop_front();
            if (bid_list.empty())
                bids_.erase(bids_.begin());
        }
        if (ask.is_filled())
        {
            order_index_.erase(ask.id);
            ask_list.pop_front();
            if (ask_list.empty())
                asks_.erase(asks_.begin());
        }
    }
    return trades;
}

bool OrderBookMap::cancel_order(uint64_t id)
{
    auto it = order_index_.find(id);
    if (it == order_index_.end())
        return false;

    auto [is_bid, list_iter] = it->second;
    double price = list_iter->price;

    if (is_bid)
    {
        bids_[price].erase(list_iter);
        if (bids_[price].empty())
            bids_.erase(price);
    }
    else
    {
        asks_[price].erase(list_iter);
        if (asks_[price].empty())
            asks_.erase(price);
    }
    order_index_.erase(it);
    return true;
}

double OrderBookMap::best_bid() const
{
    return bids_.empty() ? 0.0 : bids_.begin()->first;
}
double OrderBookMap::best_ask() const
{
    return asks_.empty() ? 0.0 : asks_.begin()->first;
}

void OrderBookMap::print_top(int levels) const
{
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << std::setw(10) << "ASK QTY"
              << std::setw(12) << "PRICE" << "\n";
    int i = 0;
    for (auto it = asks_.rbegin(); it != asks_.rend() && i < levels; ++it, ++i)
    {
        uint32_t total = 0;
        for (auto &o : it->second)
            total += o.remaining();
        std::cout << std::setw(10) << total
                  << std::setw(12) << std::fixed
                  << std::setprecision(2) << it->first << "\n";
    }
    std::cout << "  -------- spread --------\n";
    i = 0;
    for (auto &[price, lst] : bids_)
    {
        if (i++ >= levels)
            break;
        uint32_t total = 0;
        for (auto &o : lst)
            total += o.remaining();
        std::cout << std::setw(10) << total
                  << std::setw(12) << std::fixed
                  << std::setprecision(2) << price << "\n";
    }
}