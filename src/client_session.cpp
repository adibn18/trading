#include "client_session.h"
#include <iostream>
#include <utility>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)) {
}

void ClientSession::start() {
    std::cout<<"Session started with client: " << socket_.remote_endpoint() << "\n";
    while(true){
        char buffer[1024];
        boost::system::error_code erc;
        std::size_t bytes = socket_.read_some(boost::asio::buffer(buffer), erc);
        if(erc) break;
        std::string msg(buffer,bytes);
        std::cout<<"Received: " << msg << "\n";
        std::string response = "ACK\n ";
        boost::asio::write(socket_, boost::asio::buffer(response), erc);
        if(erc) break;
    }
    std::cout<<"Client disconnected: \n";
}