#pragma once
#include "report.h"
#include "metrics.h"
#include "spsc_queue.h"
#include <unordered_map>
#include <mutex>

class ReportDispatcher{
public :
    ReportDispatcher(SPSCQueue<ExecutionReport> &report_q,
                     PerformanceMetrics &metrics);
    void run();
    void RegisterClient(int trader_id,SPSCQueue<ExecutionReport> *queue);
    void unregisterClient(int trader_id);
private :
    bool pop_report(ExecutionReport& r);
    void dispatch(ExecutionReport& r);

    SPSCQueue<ExecutionReport> &report_q_;
    PerformanceMetrics &metrics_;
    std::unordered_map<int,SPSCQueue<ExecutionReport>*> clients_;
    std::mutex client_mutex_;
};
