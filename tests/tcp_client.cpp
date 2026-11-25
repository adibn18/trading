#include "tcp_client.h"

#include <iostream>
#include <stdexcept>

using boost::asio::ip::tcp;

TestClient::TestClient(const std::string& host,
                       unsigned short port)
    : socket_(io_context_),
      endpoint_(boost::asio::ip::make_address(host), port) {}

void TestClient::connect() {
    boost::system::error_code ec;

    socket_.connect(endpoint_, ec);

    if (ec) {
        throw std::runtime_error(
            "Connection failed: " + ec.message()
        );
    }
}

void TestClient::send(const std::string& message) {
    boost::system::error_code ec;

    boost::asio::write(
        socket_,
        boost::asio::buffer(message),
        ec
    );

    if (ec) {
        throw std::runtime_error(
            "Send failed: " + ec.message()
        );
    }
}

std::string TestClient::receive() {
    boost::system::error_code ec;

    char buffer[4096];

    std::size_t bytes = socket_.read_some(
        boost::asio::buffer(buffer),
        ec
    );

    if (ec) {
        throw std::runtime_error(
            "Receive failed: " + ec.message()
        );
    }

    return std::string(buffer, bytes);
}

void TestClient::close() {
    boost::system::error_code ec;

    socket_.shutdown(
        tcp::socket::shutdown_both,
        ec
    );

    socket_.close(ec);
}