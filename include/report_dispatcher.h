#pragma once
#include "report.h"
#include "spsc_queue.h"
#include <unordered_map>
#include <mutex>

class ReportDispatcher{
public :
    explicit ReportDispatcher(SPSCQueue<ExecutionReport> &report_q);
    void run();
    void RegisterClient(int trader_id,SPSCQueue<ExecutionReport> *queue);
    void unregisterClient(int trader_id);
private :
    SPSCQueue<ExecutionReport> &report_q_;
    std::unordered_map<int,SPSCQueue<ExecutionReport>*> clients_;
    std::mutex client_mutex_;
};