cmake_minimum_required(VERSION 3.16)
project(trading LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(trading
    src/main.cpp
    src/market_replay.cpp
    src/matching_engine.cpp
    src/pnl_thread.cpp
)

target_include_directories(trading PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(trading PRIVATE /W4)
else()
    target_compile_options(trading PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (MSVC)
    target_compile_options(trading PRIVATE
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(trading PRIVATE
        $<$<CONFIG:Release>:-O3>
    )
endif()



