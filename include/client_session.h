#pragma once
#include <boost/asio.hpp>
#include "report_dispatcher.h"
#include "spsc_queue.h"
#include "order.h"
#include "report.h"
#include <atomic>

class ClientSession {
    public :
        explicit ClientSession(boost::asio::ip::tcp::socket socket,int trader_id_,SPSCQueue<Order> &order_q_,ReportDispatcher &dispatcher_);
        void start();
    private :
        int trader_id_;
        boost::asio::ip::tcp::socket socket_;
        SPSCQueue<Order> &order_q_;
        SPSCQueue<ExecutionReport> report_q_;
        ReportDispatcher &dispatcher_;
        bool registered_ = false;
        void readerLoop();
        void writerLoop();
        std::atomic<bool> running_{true};
};