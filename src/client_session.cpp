#include "client_session.h"
#include <iostream>
#include "message.h"
#include "protocol.h"
#include <utility>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket,SPSCQueue<Order> &order_q,SPSCQueue<ExecutionReport> &report_q)
    : socket_(std::move(socket)),
    order_q_(order_q),
    report_q_(report_q) {
}

void ClientSession::start() {
    std::cout<<"Session started with client: " << socket_.remote_endpoint() << "\n";
    while(true){
        char buffer[1024];
        boost::system::error_code erc;
        std::size_t bytes = socket_.read_some(boost::asio::buffer(buffer), erc);
        if(erc) break;
        std::string data(buffer,bytes);
        Message msg = Protocol::parse(data);
        Order order = MessageConverter::toOrder(msg);
        while (!order_q_.push(order)) {
            ++order_q_.queue_spins_in;
        }
        std::cout<<"Order pushed"<<"\n";
        ExecutionReport r;
        while(!report_q_.pop(r)){
            ++report_q_.queue_spins_out;
        }
        std::string response = "ACK,"+ std::to_string(r.id) + "\n" ;
        boost::asio::write(socket_, boost::asio::buffer(response), erc);
        if(erc) break;
    }
    std::cout<<"Client disconnected: \n";
}