#pragma once
#include <boost/asio.hpp>
#include "spsc_queue.h"
#include "order.h"
#include "report.h"
#include "report_dispatcher.h"
#include <atomic>

class TCPServer {
    public :
        explicit TCPServer(unsigned short port,SPSCQueue<Order> &order_q_,ReportDispatcher &dispatcher_);
        void start();
    private :
        boost::asio::io_context ioContext ;
        boost::asio::ip::tcp::acceptor acceptor ;
        SPSCQueue<Order> &order_q_;
        ReportDispatcher &dispatcher_;
        std::atomic<int> next_trader_id_{0};
};