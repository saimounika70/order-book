#include "../src/order_book_map.h"
#include "../src/order_book_fast.h"
#include <chrono>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>

// measure nanoseconds for a single operation
template <typename Fn>
uint64_t time_ns(Fn &&fn)
{
    auto start = std::chrono::steady_clock::now();
    fn();
    auto end = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

struct LatencyStats
{
    double p50, p95, p99, p999, mean;
};

LatencyStats compute_stats(std::vector<uint64_t> samples)
{
    std::sort(samples.begin(), samples.end());
    size_t n = samples.size();
    double sum = 0;
    for (auto v : samples)
        sum += v;
    return {
        (double)samples[n * 50 / 100],
        (double)samples[n * 95 / 100],
        (double)samples[n * 99 / 100],
        (double)samples[n * 999 / 1000],
        sum / n};
}

// generate a realistic order stream:
// 70% limit orders, 20% cancels, 10% market orders
// prices clustered around a mid price with noise
std::vector<Order> generate_orders(int n, double mid = 100.0)
{
    std::mt19937 rng(42);
    std::normal_distribution<double> price_dist(mid, 0.5);
    std::uniform_int_distribution<uint32_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::vector<Order> orders;
    orders.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        double price = std::round(price_dist(rng) / 0.01) * 0.01;
        price = std::max(90.0, std::min(110.0, price));
        orders.push_back({static_cast<uint64_t>(i + 1),
                          side_dist(rng) ? Side::BUY : Side::SELL,
                          price,
                          qty_dist(rng),
                          0, 0});
    }
    return orders;
}

template <typename BookType>
LatencyStats bench_add_order(int n_orders)
{
    auto orders = generate_orders(n_orders);
    std::vector<uint64_t> latencies;
    latencies.reserve(n_orders);

    BookType book;
    for (auto &o : orders)
    {
        auto lat = time_ns([&]
                           { book.add_order(o); });
        latencies.push_back(lat);
    }
    return compute_stats(latencies);
}

void print_stats(const std::string &name, const LatencyStats &s)
{
    std::cout << "\n[" << name << "]\n";
    std::cout << "  mean : " << s.mean << " ns\n";
    std::cout << "  p50  : " << s.p50 << " ns\n";
    std::cout << "  p95  : " << s.p95 << " ns\n";
    std::cout << "  p99  : " << s.p99 << " ns\n";
    std::cout << "  p99.9: " << s.p999 << " ns\n";
}

void save_csv(const std::string &name,
              const LatencyStats &s,
              std::ofstream &out)
{
    out << name << ","
        << s.mean << "," << s.p50 << ","
        << s.p95 << "," << s.p99 << "," << s.p999 << "\n";
}

int main()
{
    const int N = 100'000;
    std::cout << "Benchmarking with " << N << " orders...\n";

    auto map_stats = bench_add_order<OrderBookMap>(N);
    auto fast_stats = bench_add_order<OrderBookFast>(N);

    print_stats("Baseline (std::map)", map_stats);
    print_stats("Optimised (flat array)", fast_stats);

    std::cout << "\nSpeedup p50 : "
              << map_stats.p50 / fast_stats.p50 << "x\n";
    std::cout << "Speedup p99 : "
              << map_stats.p99 / fast_stats.p99 << "x\n";

    // save for Python plotting
    std::ofstream csv("bench_results.csv");
    csv << "implementation,mean,p50,p95,p99,p999\n";
    save_csv("Baseline (std::map)", map_stats, csv);
    save_csv("Optimised (flat array)", fast_stats, csv);
    std::cout << "\nResults saved to bench_results.csv\n";
}