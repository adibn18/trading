#pragma once

#include "message.h"
#include <string>

class Protocol{
    public:
        static Message parse(const std::string & data);
};