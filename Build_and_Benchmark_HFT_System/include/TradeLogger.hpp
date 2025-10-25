#pragma once
#include <vector>
#include <fstream>
#include <string>
#include <utility>
#include <chrono>

template <typename TradeT>
class TradeLogger {
public:
    explicit TradeLogger(std::string path, std::size_t batch_size = 4096)
        : path_(std::move(path)), batch_size_(batch_size) {
        file_.open(path_, std::ios::out | std::ios::trunc);
        file_ << "buy_id,sell_id,price,quantity,timestamp_ns\n";
        buffer_.reserve(batch_size_);
    }

    ~TradeLogger() {
        flush();
        file_.close();
    }

    void reserve(std::size_t n) { buffer_.reserve(n); }

    void push(const TradeT& t) {
        buffer_.push_back(t);
        if (buffer_.size() >= batch_size_) flush();
    }

    void append(const std::vector<TradeT>& trades) {
        for (const auto& t : trades) push(t);
    }

    void flush() {
        if (buffer_.empty()) return;
        for (const auto& t : buffer_) {
            long long ts_ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    t.ts.time_since_epoch()).count();

            file_ << t.buy_id   << ','
                  << t.sell_id  << ','
                  << t.price    << ','
                  << t.quantity << ','
                  << ts_ns      << '\n';
        }
        buffer_.clear();
        file_.flush();
    }

private:
    std::string path_;
    std::size_t batch_size_;
    std::ofstream file_;
    std::vector<TradeT> buffer_;
};
