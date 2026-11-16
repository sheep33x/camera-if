#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>
template <class T> class ThreadQueue {
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> items_;
    bool closed_ = false;
public:
    void push(T item) {
        { std::lock_guard<std::mutex> lock(mutex_); if (closed_) return; items_.push(std::move(item)); }
        cv_.notify_one();
    }
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&]{ return closed_ || !items_.empty(); });
        if (items_.empty()) return std::nullopt;
        T item = std::move(items_.front()); items_.pop(); return item;
    }
    void close() {
        { std::lock_guard<std::mutex> lock(mutex_); closed_ = true; }
        cv_.notify_all();
    }
};
