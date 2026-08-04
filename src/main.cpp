#include "matching_engine.h"
#include "order.h"
#include "tcp_server.h"
#include "report.h"
#include "client_session.h"
#include <thread>
#include <iostream>
#include <iomanip>

void market_replay(SPSCQueue<Order>&);
void pnl_thread(SPSCQueue<Trade>&, LatencyStats&);

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main() {
    constexpr size_t Order_queue_size = 1<<11;
    constexpr size_t Trade_queue_size = 1<<11;
    constexpr size_t Report_queue_size = 1<<11;
    SPSCQueue<Order> order_q(Order_queue_size);
    SPSCQueue<Trade> trade_q(Trade_queue_size);
    SPSCQueue<ExecutionReport> report_q(Report_queue_size);
    uint64_t t1 = now_ns();
    
    ReportDispatcher dispatcher(report_q);
    MatchingEngine engine(order_q, trade_q,report_q);
    LatencyStats metrics_lat;

    std::thread match(&MatchingEngine::run, &engine);
    std::thread pnl(pnl_thread, std::ref(trade_q),std::ref(metrics_lat));
    std::thread dispatcherthread(&ReportDispatcher::run,&dispatcher);
    

    TCPServer server(8080,order_q,dispatcher);
    server.start();

    match.join();
    pnl.join();
    dispatcherthread.join();

    std::cout << "\n--- Latency ---\n";
    engine.ingress_lat.report("Ingress");
    engine.match_lat.report("Match");
    metrics_lat.report("Metrics");

    std::cout << "Order queue spins (in): "<< order_q.queue_spins_in << "\n";
    std::cout << "Order queue spins (out): "<< order_q.queue_spins_out << "\n";
    std::cout << "Trade queue spins (in): "<< trade_q.queue_spins_in << "\n";
    std::cout << "Trade queue spins (out): "<< trade_q.queue_spins_out << "\n";

    uint64_t t2 = now_ns();
    std::cout<<"\nTotal time for execution : " << (t2-t1) <<" ns \n";

    double throughput = (1e6/(static_cast<double>(t2-t1)) ) * 1e9;
    std::cout<<"\nThroughput for orders : " << std::fixed << std::setprecision(5) << throughput <<"\n";
}