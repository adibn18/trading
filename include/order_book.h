#pragma once
#include "order.h"
#include "report.h"
#include "spsc_queue.h"
#include "mpsc_queue.h"
#include <unordered_map>

class OrderBook {
public:
    static constexpr int MIN_PRICE = 1;
    static constexpr int MAX_PRICE = 100;

    OrderBook();
    ~OrderBook();

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) noexcept;
    OrderBook& operator=(OrderBook&&) noexcept;

    void match(Order& o, uint64_t& trade_id, SPSCQueue<Trade>& trade_q_, MPSCQueue<ExecutionReport>& report_q_);
    bool cancel(uint64_t id, MPSCQueue<ExecutionReport>& report_q_);
    bool modify(Order& o, uint64_t& trade_id, SPSCQueue<Trade>& trade_q_, MPSCQueue<ExecutionReport>& report_q_);

private:
    struct PriceLevel {
        OrderNode* head = nullptr;
        OrderNode* tail = nullptr;
    };

    static bool valid_price(int price) noexcept {
        return price >= MIN_PRICE && price <= MAX_PRICE;
    }

    void clear();
    void push_back(PriceLevel& level, OrderNode* node);
    void unlink(PriceLevel& level, OrderNode* node);
    void advance_best_bid();
    void advance_best_ask();
    void rest_order(Order& o, Side side);

    PriceLevel bids_[MAX_PRICE + 1]{};
    PriceLevel asks_[MAX_PRICE + 1]{};
    int best_bid_ = 0;                 // 0  => no bids
    int best_ask_ = MAX_PRICE + 1;     // >MAX => no asks
    std::unordered_map<uint64_t, OrderLocation> order_index_;
};
