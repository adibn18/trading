#include "tcp_server.h"
#include "client_session.h"
#include <iostream>
using boost::asio::ip::tcp;

TCPServer::TCPServer(unsigned short port,SPSCQueue<Order> &order_q,SPSCQueue<ExecutionReport> &report_q)
    :acceptor(ioContext,tcp::endpoint(tcp::v4(),port)),
    order_q_(order_q),
    report_q_(report_q){
}

void TCPServer::start(){
    std::cout<<"Trading Server Strated\n";
    std::cout<<"Listening on port 8080..\n";
    while(true){
        tcp::socket socket(ioContext);
        acceptor.accept(socket);
        std::cout<<"Client Connected : " << socket.remote_endpoint() << "\n";
        ClientSession session(std::move(socket),order_q_,report_q_);
        session.start();
    }
}