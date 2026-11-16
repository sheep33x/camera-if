#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
template <class T> class ThreadQueue {
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> items_;
public:
    void push(T item) { { std::lock_guard<std::mutex> lock(mutex_); items_.push(std::move(item)); } cv_.notify_one(); }
    T pop() { std::unique_lock<std::mutex> lock(mutex_); cv_.wait(lock, [&]{return !items_.empty();}); T v = std::move(items_.front()); items_.pop(); return v; }
};
