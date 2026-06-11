#pragma once
#include <cstdint>
#include <string>

enum class Side
{
    BUY,
    SELL
};
enum class OrderType
{
    LIMIT,
    MARKET
};

struct Order
{
    uint64_t id;
    Side side;
    double price;
    uint32_t quantity;  // total quantity
    uint32_t filled;    // how much has been matched so far
    uint64_t timestamp; // arrival time in nanoseconds

    uint32_t remaining() const { return quantity - filled; }
    bool is_filled() const { return filled >= quantity; }
};

struct Trade
{
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint32_t quantity;
    uint64_t timestamp;
};