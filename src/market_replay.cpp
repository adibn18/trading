#include "spsc_queue.h"
#include "order.h"
#include <random>
#include <chrono>

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void market_replay(SPSCQueue<Order>& q) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> price(100,125);
    std::uniform_int_distribution<int> qty(1,1000);
    std::uniform_int_distribution<int> side(0,1);
    std::uniform_int_distribution<int> type(0,1);
    std::uniform_int_distribution<int> sym(0,4);
    std::uniform_int_distribution<int> trader(1,3);

    for (uint64_t i=0;i<1000000;i++) {
        Order o{
            i, trader(rng),
            Symbol(sym(rng)),
            side(rng)?Side::BUY:Side::SELL,
            type(rng)?OrderType::LIMIT:OrderType::MARKET,
            price(rng), qty(rng),
            now_ns(),0,0
        };
        if(o.type == OrderType::MARKET) o.price = 0; 
        while (!q.push(o)) {
            ++q.queue_spins_in;
        }
    }

    Order shutdown{};
    shutdown.type = OrderType::SHUTDOWN;
    while (!q.push(shutdown)) {
        ++q.queue_spins_in;
    }
}

