#include "tcp_server.h"
#include "client_session.h"
#include <iostream>
#include "report_dispatcher.h"
#include <memory>
#include <thread>
using boost::asio::ip::tcp;

TCPServer::TCPServer(unsigned short port,SPSCQueue<Order> &order_q,ReportDispatcher &dispatcher)
    :acceptor(ioContext,tcp::endpoint(tcp::v4(),port)),
    order_q_(order_q),
    dispatcher_(dispatcher){
}

void TCPServer::start(){
    std::cout<<"Trading Server Strated\n";
    std::cout<<"Listening on port 8080..\n";
    while(true){
        tcp::socket socket(ioContext);
        acceptor.accept(socket);
        std::cout<<"Client Connected : " << socket.remote_endpoint() << "\n";
        int trader_id = next_trader_id_++;
        auto session = std::make_shared<ClientSession>(std::move(socket),trader_id,order_q_,dispatcher_);
        std::thread(&ClientSession::start,session).detach();
    }
}

ReportDispatcher::ReportDispatcher(SPSCQueue<ExecutionReport> &report_q) : report_q_(report_q){

}

void ReportDispatcher::run(){
    while (true){
        ExecutionReport r;
        while (!report_q_.pop(r)){
            std::this_thread::yield();
        }
        SPSCQueue<ExecutionReport>* queue = nullptr;
        {
            std::lock_guard<std::mutex> lock(client_mutex_);
            auto it = clients_.find(r.trader_id);
            if(it != clients_.end()) queue = it->second;
        }
        if(queue){
            while(!queue->push(r)){
                std::this_thread::yield();
            }
        }
    } 
}

void ReportDispatcher::RegisterClient(int trader_id,SPSCQueue<ExecutionReport> *queue){
    std::lock_guard<std::mutex> lock(client_mutex_);
    clients_[trader_id] = queue;
}

void ReportDispatcher::unregisterClient(int trader_id){
    std::lock_guard<std::mutex> lock(client_mutex_);
    clients_.erase(trader_id);
}