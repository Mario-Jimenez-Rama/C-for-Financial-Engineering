#pragma once
#include <chrono>

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;

    void start() noexcept {
        start_ = Clock::now();
    }

    // Returns elapsed nanoseconds since start()
    long long stop() const noexcept {
        auto end = Clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
    }

private:
    Clock::time_point start_{Clock::now()};
};
