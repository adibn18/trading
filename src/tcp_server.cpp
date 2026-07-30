#include "tcp_server.h"
#include <iostream>
using boost::asio::ip::tcp;

TCPServer::TCPServer(unsigned short port)
    :acceptor(ioContext,tcp::endpoint(tcp::v4(),port)){
}

void TCPServer::start(){
    std::cout<<"Trading Server Strated\n";
    std::cout<<"Listening on port 8080..\n";
    while(true){
        tcp::socket socket(ioContext);
        acceptor.accept(socket);
        std::cout<<"Client Connected : " << socket.remote_endpoint() << std::endl;
    }
}