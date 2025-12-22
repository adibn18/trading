#pragma once
#include "order.h"
#include <map>
#include <deque>
#include <vector>

class OrderBook {
public:
    void match(Order& o,uint64_t& trade_id,SPSCQueue<Trade>& trade_q_);
private:
    std::map<int, std::deque<Order>, std::greater<int>> bids_;
    std::map<int, std::deque<Order>> asks_;
};
