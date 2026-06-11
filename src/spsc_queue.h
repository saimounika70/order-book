#pragma once
#include <atomic>
#include <array>
#include <optional>

// Single Producer Single Consumer lock-free queue
// Cache-line aligned to prevent false sharing between
// producer writing tail and consumer reading head

template <typename T, size_t CAPACITY>
class SPSCQueue
{
    static_assert((CAPACITY & (CAPACITY - 1)) == 0,
                  "CAPACITY must be power of 2");

public:
    // called by producer thread only
    bool push(const T &item)
    {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) & MASK;
        if (next == head_.load(std::memory_order_acquire))
            return false; // queue full
        buffer_[tail] = item;
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // called by consumer thread only
    std::optional<T> pop()
    {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire))
            return std::nullopt; // queue empty
        T item = buffer_[head];
        head_.store((head + 1) & MASK, std::memory_order_release);
        return item;
    }

    bool empty() const
    {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t MASK = CAPACITY - 1;

    // each on its own cache line to prevent false sharing
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    alignas(64) std::array<T, CAPACITY> buffer_{};
};