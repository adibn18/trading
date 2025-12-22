#include "matching_engine.h"
#include "order.h"
#include <thread>
#include <iostream>

void market_replay(SPSCQueue<Order>&);
void pnl_thread(SPSCQueue<Trade>&, LatencyStats&);

int main() {
    constexpr size_t Order_queue_size = 1<<11;
    constexpr size_t Trade_queue_size = 1<<11;
    SPSCQueue<Order> order_q(Order_queue_size);
    SPSCQueue<Trade> trade_q(Trade_queue_size);

    MatchingEngine engine(order_q, trade_q);
    LatencyStats metrics_lat;

    std::thread prod(market_replay, std::ref(order_q));
    std::thread match(&MatchingEngine::run, &engine);
    std::thread pnl(pnl_thread, std::ref(trade_q),std::ref(metrics_lat));

    prod.join();
    match.join();
    pnl.join();

    std::cout << "\n--- Latency ---\n";
    engine.ingress_lat.report("Ingress");
    engine.match_lat.report("Match");
    metrics_lat.report("Metrics");

    std::cout << "Order queue spins (in): "<< order_q.queue_spins_in << "\n";
    std::cout << "Order queue spins (out): "<< order_q.queue_spins_out << "\n";
    std::cout << "Trade queue spins (in): "<< trade_q.queue_spins_in << "\n";
    std::cout << "Trade queue spins (out): "<< trade_q.queue_spins_out << "\n";
}