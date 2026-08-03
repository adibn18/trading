#include "message.h"
#include <chrono>

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

Order MessageConverter::toOrder(const Message &msg){
    Order order;
    order.id = msg.orderId;
    order.symbol = msg.symbol;
    order.trader_id = 1;
    order.side = msg.side;
    order.type = msg.orderType;
    order.price = msg.price;
    order.qty = msg.quantity;
    order.t_created = now_ns();
    order.t_emitted = 0;
    order.t_matched = 0;
    return order;
}