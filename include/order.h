#pragma once

#include <cstdint>
#include <string>
enum class Side { BUY, SELL };
enum class OrderType { LIMIT , MARKET, SHUTDOWN };

struct Order {
    uint64_t id;
    int trader_id;
    std::string symbol;
    Side side;
    OrderType type;
    int price;
    int qty;
    uint64_t t_created;
    uint64_t t_emitted;
    uint64_t t_matched;
};

struct Trade {
    uint64_t trade_id;
    int trade_buyer_id;
    int trade_seller_id;
    std::string symbol;
    int price;
    int qty;
    uint64_t t_created;
    uint64_t t_emitted;
};


