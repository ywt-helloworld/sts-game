#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace sts {

template <typename T>
class ThreadSafeMessageQueue {
public:
    void push(T value) {
        std::lock_guard lock(mutex_);
        queue_.push(std::move(value));
    }

    [[nodiscard]] std::optional<T> tryPop() {
        std::lock_guard lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

private:
    std::mutex mutex_;
    std::queue<T> queue_;
};

} // namespace sts
