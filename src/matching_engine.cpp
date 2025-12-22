#include <matching_engine.h>
#include <chrono>

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void OrderBook::match(Order& o,uint64_t& trade_id,SPSCQueue<Trade>& trade_q_){
    if (o.side == Side::BUY) {
        auto& book = asks_;
        while (o.qty > 0 && !book.empty()) {
            auto it = book.begin();
            if (o.type == OrderType::LIMIT && it->first > o.price) break;
            auto& resting = it->second.front();
            int traded = std::min(o.qty, resting.qty);
            o.qty -= traded;
            resting.qty -= traded;
            int price = (o.type == OrderType::LIMIT) ? o.price : resting.price;
            Trade t{
                trade_id++,
                o.trader_id,resting.trader_id,
                o.symbol,price,
                traded,
                now_ns(),0
            };
            while (!trade_q_.push(t)){
                ++trade_q_.queue_spins_in;
            }
            if (resting.qty == 0){
                it->second.pop_front();
                if (it->second.empty()) book.erase(it);
            }
        }
        if (o.qty > 0 && o.type == OrderType::LIMIT) bids_[o.price].push_back(o);
    }
    else {
        auto& book = bids_;
        while (o.qty > 0 && !book.empty()) {
            auto it = book.begin();
            if (o.type == OrderType::LIMIT && it->first < o.price) break;
            auto& resting = it->second.front();
            int traded = std::min(o.qty, resting.qty);
            o.qty -= traded;
            resting.qty -= traded;
            int price = (o.type == OrderType::LIMIT) ? o.price : resting.price;
            Trade t{
                trade_id++,
                resting.trader_id,o.trader_id,
                o.symbol,price,
                traded,
                now_ns(),0
            };
            while (!trade_q_.push(t)){
                ++trade_q_.queue_spins_in;
            }
            if (resting.qty == 0){
                it->second.pop_front();
                if (it->second.empty()) book.erase(it);
            }
        }
        if (o.qty > 0 && o.type == OrderType::LIMIT) asks_[o.price].push_back(o);
    }
}

void MatchingEngine::run(){
    Order o;
    uint64_t trade_id = 0;
    uint64_t process = 0;
    constexpr uint64_t Warmup = 1000;
    while (true) {
        if (!order_q_.pop(o)){
            ++order_q_.queue_spins_out;
            continue;
        }
        if (o.type == OrderType::SHUTDOWN) break;
        o.t_emitted = now_ns();
        books_[o.symbol].match(o,trade_id,trade_q_);
        o.t_matched = now_ns();
        if(++process > Warmup){
            match_lat.add(o.t_matched-o.t_emitted);
            ingress_lat.add(o.t_emitted-o.t_created);
        }
    }
    std::cout<<"No. of trades == "<<trade_id<<"\n";
    Trade shutdown{};
    shutdown.trade_buyer_id = -1;
    while (!trade_q_.push(shutdown)){
        ++trade_q_.queue_spins_in;
    }
}