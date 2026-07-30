#pragma once
#include <boost/asio.hpp>

class TCPServer {
    public :
        explicit TCPServer(unsigned short port);
        void start();
    private :
        boost::asio::io_context ioContext ;
        boost::asio::ip::tcp::acceptor acceptor ;
};