#include <matching_engine.h>
#include <report.h>
#include <iostream>
#include <thread>
#include <utility>
#include <rdtsc.h>

OrderBook::OrderBook() = default;

OrderBook::~OrderBook() {
    clear();
}

OrderBook::OrderBook(OrderBook&& other) noexcept
    : best_bid_(other.best_bid_),
      best_ask_(other.best_ask_),
      order_index_(std::move(other.order_index_)) {
    for (int p = MIN_PRICE; p <= MAX_PRICE; ++p) {
        bids_[p] = other.bids_[p];
        asks_[p] = other.asks_[p];
        other.bids_[p] = {};
        other.asks_[p] = {};
    }
    other.best_bid_ = 0;
    other.best_ask_ = MAX_PRICE + 1;
}

OrderBook& OrderBook::operator=(OrderBook&& other) noexcept {
    if (this == &other) return *this;
    clear();
    best_bid_ = other.best_bid_;
    best_ask_ = other.best_ask_;
    order_index_ = std::move(other.order_index_);
    for (int p = MIN_PRICE; p <= MAX_PRICE; ++p) {
        bids_[p] = other.bids_[p];
        asks_[p] = other.asks_[p];
        other.bids_[p] = {};
        other.asks_[p] = {};
    }
    other.best_bid_ = 0;
    other.best_ask_ = MAX_PRICE + 1;
    return *this;
}

void OrderBook::clear() {
    for (int p = MIN_PRICE; p <= MAX_PRICE; ++p) {
        for (OrderNode* n = bids_[p].head; n; ) {
            OrderNode* next = n->next;
            delete n;
            n = next;
        }
        bids_[p] = {};
        for (OrderNode* n = asks_[p].head; n; ) {
            OrderNode* next = n->next;
            delete n;
            n = next;
        }
        asks_[p] = {};
    }
    best_bid_ = 0;
    best_ask_ = MAX_PRICE + 1;
    order_index_.clear();
}

void OrderBook::push_back(PriceLevel& level, OrderNode* node) {
    node->prev = level.tail;
    node->next = nullptr;
    if (level.tail) level.tail->next = node;
    else level.head = node;
    level.tail = node;
}

void OrderBook::unlink(PriceLevel& level, OrderNode* node) {
    if (node->prev) node->prev->next = node->next;
    else level.head = node->next;
    if (node->next) node->next->prev = node->prev;
    else level.tail = node->prev;
    node->prev = nullptr;
    node->next = nullptr;
}

void OrderBook::advance_best_bid() {
    while (best_bid_ >= MIN_PRICE && !bids_[best_bid_].head) {
        --best_bid_;
    }
    if (best_bid_ < MIN_PRICE) best_bid_ = 0;
}

void OrderBook::advance_best_ask() {
    while (best_ask_ <= MAX_PRICE && !asks_[best_ask_].head) {
        ++best_ask_;
    }
}

void OrderBook::rest_order(Order& o, Side side) {
    if (!valid_price(o.price) || o.qty <= 0) return;
    auto* node = new OrderNode{o, nullptr, nullptr};
    if (side == Side::BUY) {
        push_back(bids_[o.price], node);
        if (o.price > best_bid_) best_bid_ = o.price;
    } else {
        push_back(asks_[o.price], node);
        if (o.price < best_ask_) best_ask_ = o.price;
    }
    order_index_[o.id] = {side, o.price, node};
}

void OrderBook::match(Order& o,uint64_t& trade_id,SPSCQueue<Trade>& trade_q_,MPSCQueue<ExecutionReport>& report_q_){
    if (o.side == Side::BUY) {
        while (o.qty > 0 && best_ask_ <= MAX_PRICE) {
            if (o.type == OrderType::LIMIT && best_ask_ > o.price) break;
            PriceLevel& level = asks_[best_ask_];
            OrderNode* node = level.head;
            Order& resting = node->order;
            int traded = std::min(o.qty, resting.qty);
            o.qty -= traded;
            resting.qty -= traded;
            int price = (o.type == OrderType::LIMIT) ? o.price : resting.price;
            Trade t{
                trade_id++,
                o.trader_id,resting.trader_id,
                o.symbol,price,
                traded,
                now_tsc(),0
            };
            while (!trade_q_.push(t)){
                ++trade_q_.queue_spins_in;
                std::this_thread::yield();
            }
            ExecutionReport r1;
            r1.trader_id = resting.trader_id;
            r1.fillqty = traded;
            r1.price = price;
            r1.id = resting.id;
            r1.remqty = resting.qty;
            r1.symbol = resting.symbol;
            if(r1.remqty == 0){
                r1.type = ExecType::FILL;
            }
            else {
                r1.type = ExecType::PARTIAL_FILL;
            }
            while (!report_q_.push(r1)){
                ++report_q_.queue_spins_in;
                std::this_thread::yield();
            }
            ExecutionReport r2;
            if(o.qty > 0) r2.type = ExecType::PARTIAL_FILL;
            else r2.type = ExecType::FILL;
            r2.trader_id = o.trader_id;
            r2.fillqty = traded;
            r2.price = price;
            r2.id = o.id;
            r2.remqty = o.qty - traded;
            r2.symbol = o.symbol;
            while (!report_q_.push(r2)){
                ++report_q_.queue_spins_in;
                std::this_thread::yield();
            }
            if (resting.qty == 0){
                const uint64_t filled_id = resting.id;
                unlink(level, node);
                order_index_.erase(filled_id);
                delete node;
                if (!level.head) advance_best_ask();
            }
        }
        if (o.qty > 0 && o.type == OrderType::LIMIT){
            rest_order(o, Side::BUY);
        }
    }
    else {
        while (o.qty > 0 && best_bid_ >= MIN_PRICE) {
            if (o.type == OrderType::LIMIT && best_bid_ < o.price) break;
            PriceLevel& level = bids_[best_bid_];
            OrderNode* node = level.head;
            Order& resting = node->order;
            int traded = std::min(o.qty, resting.qty);
            o.qty -= traded;
            resting.qty -= traded;
            int price = (o.type == OrderType::LIMIT) ? o.price : resting.price;
            Trade t{
                trade_id++,
                resting.trader_id,o.trader_id,
                o.symbol,price,
                traded,
                now_tsc(),0
            };
            while (!trade_q_.push(t)){
                ++trade_q_.queue_spins_in;
                std::this_thread::yield();
            }
            ExecutionReport r1;
            r1.trader_id = resting.trader_id;
            r1.fillqty = traded;
            r1.price = price;
            r1.id = resting.id;
            r1.remqty = resting.qty;
            r1.symbol = resting.symbol;
            if(r1.remqty == 0){
                r1.type = ExecType::FILL;
            }
            else {
                r1.type = ExecType::PARTIAL_FILL;
            }
            while (!report_q_.push(r1)){
                ++report_q_.queue_spins_in;
                std::this_thread::yield();
            }
            ExecutionReport r2;
            if(o.qty > 0) r2.type = ExecType::PARTIAL_FILL;
            else r2.type = ExecType::FILL;
            r2.trader_id = o.trader_id;
            r2.fillqty = traded;
            r2.price = price;
            r2.id = o.id;
            r2.remqty = o.qty - traded;
            r2.symbol = o.symbol;
            while (!report_q_.push(r2)){
                ++report_q_.queue_spins_in;
                std::this_thread::yield();
            }
            if (resting.qty == 0){
                const uint64_t filled_id = resting.id;
                unlink(level, node);
                order_index_.erase(filled_id);
                delete node;
                if (!level.head) advance_best_bid();
            }
        }
        if (o.qty > 0 && o.type == OrderType::LIMIT){
            rest_order(o, Side::SELL);
        }
    }
}

bool OrderBook::cancel(uint64_t id,MPSCQueue<ExecutionReport>& report_q_){
    auto idx = order_index_.find(id);
    if(idx == order_index_.end()) return false;
    auto &loc = idx->second;
    OrderNode* node = loc.node;
    if (!node) return false;
    const Order cancelled = node->order;

    PriceLevel& level = (loc.side == Side::BUY) ? bids_[loc.price] : asks_[loc.price];
    unlink(level, node);
    if (!level.head) {
        if (loc.side == Side::BUY) {
            if (loc.price == best_bid_) advance_best_bid();
        } else {
            if (loc.price == best_ask_) advance_best_ask();
        }
    }
    order_index_.erase(idx);
    delete node;

    ExecutionReport r;
    r.type = ExecType::CANCELLED;
    r.id = id;
    r.symbol = cancelled.symbol;
    r.trader_id = cancelled.trader_id;
    r.remqty = cancelled.qty;
    while(!report_q_.push(r)){
        ++report_q_.queue_spins_in;
        std::this_thread::yield();
    }
    return true;
}

bool OrderBook::modify(Order& o,uint64_t& trade_id,SPSCQueue<Trade>& trade_q_,MPSCQueue<ExecutionReport>& report_q_){
    auto idx = order_index_.find(o.id);
    if(idx == order_index_.end()) return false;
    if(o.qty <= 0) return false;

    auto &loc = idx->second;
    OrderNode* node = loc.node;
    if (!node) return false;
    Order& resting = node->order;
    const int old_price = resting.price;

    auto push_modified = [&](const Order& updated){
        ExecutionReport r;
        r.type = ExecType::MODIFIED;
        r.id = updated.id;
        r.symbol = updated.symbol;
        r.trader_id = updated.trader_id;
        r.price = updated.price;
        r.remqty = updated.qty;
        while(!report_q_.push(r)){
            ++report_q_.queue_spins_in;
            std::this_thread::yield();
        }
    };

    if(o.price == old_price){
        resting.qty = o.qty;
        push_modified(resting);
        return true;
    }

    if (!valid_price(o.price)) return false;

    Order updated = resting;
    updated.price = o.price;
    updated.qty = o.qty;
    const Side side = loc.side;

    PriceLevel& level = (side == Side::BUY) ? bids_[loc.price] : asks_[loc.price];
    unlink(level, node);
    if (!level.head) {
        if (side == Side::BUY) {
            if (loc.price == best_bid_) advance_best_bid();
        } else {
            if (loc.price == best_ask_) advance_best_ask();
        }
    }
    order_index_.erase(idx);
    delete node;

    match(updated,trade_id,trade_q_,report_q_);
    push_modified(updated);
    return true;
}

void MatchingEngine::run(){
    Order o;
    uint64_t trade_id = 0;
    uint64_t process = 0;
    constexpr uint64_t Warmup = 1000;
    ingress_lat.t_started = now_tsc();
    while (true) {
        if (!order_q_.pop(o)){
            ++order_q_.queue_spins_out;
            std::this_thread::yield();
            continue;
        }
        if (o.type == OrderType::SHUTDOWN) break;
        o.t_emitted = now_tsc();
        ExecutionReport report;
        report.type = ExecType::ACK;
        report.id = o.id;
        report.symbol = o.symbol;
        report.trader_id = o.trader_id;
        while(!(report_q_.push(report))){
            ++report_q_.queue_spins_in;
            std::this_thread::yield();
        }
        if(o.rtype == RequestType::CANCEL){
            if(!books_[o.symbol].cancel(o.id,report_q_)){
                ExecutionReport reject;
                reject.type = ExecType::REJECT;
                reject.id = o.id;
                reject.symbol = o.symbol;
                reject.trader_id = o.trader_id;
                while(!report_q_.push(reject)){
                    ++report_q_.queue_spins_in;
                    std::this_thread::yield();
                }
            }
            continue;
        }
        if(o.rtype == RequestType::MODIFY){
            if(!books_[o.symbol].modify(o,trade_id,trade_q_,report_q_)){
                ExecutionReport reject;
                reject.type = ExecType::REJECT;
                reject.id = o.id;
                reject.symbol = o.symbol;
                reject.trader_id = o.trader_id;
                while(!report_q_.push(reject)){
                    ++report_q_.queue_spins_in;
                    std::this_thread::yield();
                }
            }
            continue;
        }
        books_[o.symbol].match(o,trade_id,trade_q_,report_q_);
        o.t_matched = now_tsc();
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
        std::this_thread::yield();
    }
    ExecutionReport shutdown_r{};
    shutdown_r.trader_id = -1;
    while (!report_q_.push(shutdown_r)){
        ++report_q_.queue_spins_in;
        std::this_thread::yield();
    }
    ingress_lat.t_ended = now_tsc();
}
