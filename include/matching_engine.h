#pragma once

#include "spsc_queue.h"
#include "order_book.h"
#include "latency.h"
#include <unordered_map>

class MatchingEngine {
public:
    MatchingEngine(SPSCQueue<Order>& oq,SPSCQueue<Trade>& tq): order_q_(oq), trade_q_(tq) {}
    void run();
    LatencyStats ingress_lat;
    LatencyStats match_lat;
    SPSCQueue<Trade>& trade_q_;
private:
    SPSCQueue<Order>& order_q_;
    std::unordered_map<Symbol, OrderBook> books_;
};

