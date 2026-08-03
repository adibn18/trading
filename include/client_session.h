#pragma once
#include <boost/asio.hpp>
#include "spsc_queue.h"
#include "order.h"
#include "report.h"

class ClientSession {
    public :
        explicit ClientSession(boost::asio::ip::tcp::socket socket,SPSCQueue<Order> &order_q_,SPSCQueue<ExecutionReport> &report_q_);
        void start();
    private :
        boost::asio::ip::tcp::socket socket_;
        SPSCQueue<Order> &order_q_;
        SPSCQueue<ExecutionReport> &report_q_;
};