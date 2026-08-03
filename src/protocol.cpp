#include "protocol.h"

#include <sstream>
#include <vector>

Message Protocol::parse(const std::string &data){
    Message msg;
    std::stringstream ss(data);
    std::string token;
    std::vector<std::string> fields;
    while(std::getline(ss,token,',')){
        fields.push_back(token);
    }
    if(fields.size() != 6) return msg;
    if(fields[0] == "NEW") msg.type = MessageType::NEW_ORDER;
    msg.orderId = std::stoi(fields[1]);
    if(fields[2] == "BUY") msg.side = Side::BUY;
    else msg.side = Side::SELL;
    msg.symbol = fields[3];
    msg.quantity =std::stoi(fields[4]);
    msg.price = std::stoi(fields[5]);
    return msg;
}