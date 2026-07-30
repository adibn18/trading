#pragma once
#include <boost/asio.hpp>

class ClientSession {
    public :
        explicit ClientSession(boost::asio::ip::tcp::socket socket);
        void start();
    private :
        boost::asio::ip::tcp::socket socket_;
};