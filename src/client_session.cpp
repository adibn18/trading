#include "client_session.h"
#include <iostream>
#include <thread>
#include "message.h"
#include "protocol.h"
#include <utility>

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket,int trader_id,SPSCQueue<Order> &order_q,SPSCQueue<ExecutionReport> &report_q)
    : socket_(std::move(socket)),
    order_q_(order_q),
    report_q_(report_q),
    trader_id_(trader_id) {
}

void ClientSession::start() {
    std::cout<<"Session started with client: " << socket_.remote_endpoint() << "\n";
    std::thread writer(&ClientSession::writerLoop,this);
    readerLoop();
    running_ = false;
    writer.join();
    std::cout<<"Client disconnected: \n";
}

void ClientSession::readerLoop(){
    while(running_){
        char buffer[1024];
        boost::system::error_code erc;
        std::size_t bytes = socket_.read_some(boost::asio::buffer(buffer), erc);
        if(erc){
            running_ = false;
            break;
        }
        std::string data(buffer,bytes);
        Message msg = Protocol::parse(data);
        Order order = MessageConverter::toOrder(msg);
        order.trader_id = trader_id_;
        while (!order_q_.push(order)) {
            ++order_q_.queue_spins_in;
        }
        std::cout<<"Order pushed"<<"\n";
    }
}

void ClientSession::writerLoop(){
    while(running_){
        boost::system::error_code erc;
        ExecutionReport r;
        while(!report_q_.pop(r)){
            ++report_q_.queue_spins_out;
        }
        std::string response;
        switch (r.type){
            case ExecType::ACK :
                response = "ACK," + std::to_string(r.id) + "\n";
                break;
            case ExecType::FILL :
                response = "FILL," + std::to_string(r.id) +"," + std::to_string(r.fillqty)+"," + std::to_string(r.price) + "\n";
                break;
            case ExecType::PARTIAL_FILL :
                response = "PARTIAL_FILL," + std::to_string(r.id)+"," + std::to_string(r.fillqty)+"," + std::to_string(r.price) + "\n";
                break;
            default : 
                continue;
        }
        boost::asio::write(socket_, boost::asio::buffer(response), erc);
        if(erc){
            running_ = false;
            break;
        }
    }
}