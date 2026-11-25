#pragma once

#include <boost/asio.hpp>
#include <string>

class TestClient {
public:
    TestClient(const std::string& host = "127.0.0.1",unsigned short port = 8080);

    void connect();
    void send(const std::string& message);
    std::string receive();
    void close();

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::endpoint endpoint_;
};