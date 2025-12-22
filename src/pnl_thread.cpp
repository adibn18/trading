#include "spsc_queue.h"
#include "order.h"
#include "latency.h"
#include <unordered_map>
#include <iostream>
#include <chrono>

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct PnL {
    int64_t cash = 0;
    int64_t pos = 0;
};

void pnl_thread(SPSCQueue<Trade>& tq,LatencyStats& metrics_lat) {
    std::unordered_map<int,std::unordered_map<Symbol, PnL>> pnl;
    Trade t;
    uint64_t process = 0;
    constexpr uint64_t Warmup = 1000;
    while (true) {
        if (!tq.pop(t)){
            ++tq.queue_spins_out;
            continue;
        }
        if (t.trade_buyer_id == -1 || t.trade_seller_id == -1) break;
        auto& p1 = pnl[t.trade_buyer_id][t.symbol];
        auto& p2 = pnl[t.trade_seller_id][t.symbol];
        p1.cash -= int64_t(t.price) * t.qty;
        p1.pos += t.qty;
        p2.cash += int64_t(t.price) * t.qty;
        p2.pos -= t.qty;
        t.t_emitted = now_ns();
        if(++process > Warmup){
            metrics_lat.add(t.t_emitted - t.t_created);
        }
    }

    std::cout << "\n--- PnL ---\n";
    for (auto& [tr, mp] : pnl)
        for (auto& [sym, p] : mp)
            std::cout << "Trader " << tr
                      << " Sym " << static_cast<char>(static_cast<int>(sym)+'A')
                      << " Cash " << p.cash
                      << " Pos " << p.pos << "\n";
}


