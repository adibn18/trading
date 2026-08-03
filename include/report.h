#pragma once

#include <cstdint>
#include <string>

enum class ExecType{
    ACK,
    FILL,
    PARTIAL_FILL,
    CANCELLED,
    REJECT
};

struct ExecutionReport{
    ExecType type;
    uint64_t id;
    std::string symbol;
    int fillqty = 0;
    int remqty = 0;
    int price = 0;
    int trader_id ;
};