#pragma once

#include <vector>
#include <algorithm>
#include <iostream>

class LatencyStats {
public :
    std::vector<uint64_t> v;
    void add(uint64_t ns){
        v.push_back(ns); 
    }
    void report(const char* name) {
        if (v.empty()) return;
        std::sort(v.begin(), v.end());
        std::cout << name <<"\n";
        std::cout << " p50=" << v[v.size()/2]<<" ns \n";
        std::cout << " p99=" << v[v.size()*99/100]<<" ns \n";
        std::cout << " max=" << v.back()<<" ns \n";
        std::cout << "\n----------------------\n";
    }
};

