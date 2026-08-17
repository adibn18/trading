#include "spsc_queue.h"
#include "mpsc_queue.h"
#include "order.h"
#include "latency.h"
#include "report.h"
#include "rdtsc.h"
#include <unordered_map>
#include <iostream>

struct PnL {
    int64_t cash = 0;
    int64_t pos = 0;
};

void pnl_thread(SPSCQueue<Trade>& tq, MPSCQueue<ExecutionReport>& report_q,LatencyStats& metrics_lat) {
    std::unordered_map<int,std::unordered_map<std::string, PnL>> pnl;
    Trade t;
    uint64_t process = 0;
    constexpr uint64_t Warmup = 0;
    while (true) {
        if (!tq.pop(t)){
            ++tq.queue_spins_out;
            continue;
        }
        if (t.trade_buyer_id == -1 || t.trade_seller_id == -1){
            break;
        }

        auto& buyer = pnl[t.trade_buyer_id][t.symbol];
        auto& seller = pnl[t.trade_seller_id][t.symbol];
        buyer.cash -= int64_t(t.price) * t.qty;
        buyer.pos += t.qty;
        seller.cash += int64_t(t.price) * t.qty;
        seller.pos -= t.qty;

        const uint64_t done = now_tsc();
        if(++process > Warmup){
            metrics_lat.add(done - t.t_created);
        }
    }

    std::cout << "\n--- PnL ---\n";
    for (auto& [tr, mp] : pnl){
        for (auto& [sym, p] : mp){
            std::cout << "Trader " << tr
                      << " Sym " << sym
                      << " Cash " << p.cash
                      << " Pos " << p.pos << "\n";
        }
    }
}
