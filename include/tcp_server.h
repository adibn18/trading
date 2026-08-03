#pragma once
#include <boost/asio.hpp>
#include "spsc_queue.h"
#include "order.h"
#include "report.h"

class TCPServer {
    public :
        explicit TCPServer(unsigned short port,SPSCQueue<Order> &order_q_,SPSCQueue<ExecutionReport> &report_q_);
        void start();
    private :
        boost::asio::io_context ioContext ;
        boost::asio::ip::tcp::acceptor acceptor ;
        SPSCQueue<Order> &order_q_;
        SPSCQueue<ExecutionReport> &report_q_;
};