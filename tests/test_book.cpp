// tests/test_book.cpp
#include "../src/order_book_map.h"
#include <cassert>
#include <iostream>
#include <cmath>

void test_simple_match()
{
    OrderBookMap book;
    // buy 100 @ 100.0, sell 100 @ 100.0 → should match immediately
    auto t1 = book.add_order({1, Side::BUY, 100.0, 100, 0, 0});
    auto t2 = book.add_order({2, Side::SELL, 100.0, 100, 0, 0});
    assert(t2.size() == 1);
    assert(t2[0].quantity == 100);
    assert(t2[0].price == 100.0);
    std::cout << "PASS: simple_match\n";
}

void test_partial_fill()
{
    OrderBookMap book;
    book.add_order({1, Side::BUY, 100.0, 200, 0, 0});               // buy 200
    auto trades = book.add_order({2, Side::SELL, 100.0, 50, 0, 0}); // sell 50
    assert(trades.size() == 1);
    assert(trades[0].quantity == 50);
    // 150 should remain on bid side
    assert(book.best_bid() == 100.0);
    std::cout << "PASS: partial_fill\n";
}

void test_no_match()
{
    OrderBookMap book;
    book.add_order({1, Side::BUY, 99.0, 100, 0, 0});
    auto t = book.add_order({2, Side::SELL, 101.0, 100, 0, 0});
    assert(t.empty()); // no match, spread exists
    assert(book.best_bid() == 99.0);
    assert(book.best_ask() == 101.0);
    std::cout << "PASS: no_match\n";
}

void test_price_time_priority()
{
    OrderBookMap book;
    // two bids at same price — earlier one should match first
    book.add_order({1, Side::BUY, 100.0, 50, 0, 0}); // arrives first
    book.add_order({2, Side::BUY, 100.0, 50, 0, 0}); // arrives second
    auto trades = book.add_order({3, Side::SELL, 100.0, 50, 0, 0});
    assert(trades[0].buy_order_id == 1); // order 1 matched, not 2
    std::cout << "PASS: price_time_priority\n";
}

void test_cancel()
{
    OrderBookMap book;
    book.add_order({1, Side::BUY, 100.0, 100, 0, 0});
    assert(book.cancel_order(1) == true);
    assert(book.cancel_order(1) == false); // already cancelled
    assert(book.best_bid() == 0.0);        // book empty
    std::cout << "PASS: cancel\n";
}

void test_multi_level_match()
{
    OrderBookMap book;
    book.add_order({1, Side::BUY, 102.0, 100, 0, 0});
    book.add_order({2, Side::BUY, 101.0, 100, 0, 0});
    book.add_order({3, Side::BUY, 100.0, 100, 0, 0});
    auto trades = book.add_order({4, Side::SELL, 100.0, 250, 0, 0});
    assert(trades.size() == 3);

    // use epsilon comparison instead of == for floating point
    auto approx_eq = [](double a, double b)
    {
        return std::abs(a - b) < 1e-9;
    };
    assert(approx_eq(trades[0].price, 102.0));
    assert(approx_eq(trades[1].price, 101.0));
    assert(approx_eq(trades[2].price, 100.0));
    std::cout << "PASS: multi_level_match\n";
}

int main()
{
    test_simple_match();
    test_partial_fill();
    test_no_match();
    test_price_time_priority();
    test_cancel();
    test_multi_level_match();
    std::cout << "\nAll tests passed.\n";
}