cmake_minimum_required(VERSION 3.16)
project(Build_and_Benchmark_HFT_System LANGUAGES CXX)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_executable(hft_app
    src/main.cpp
    src/MarketData.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/OrderManager.cpp
    src/TradeLogger.cpp
)

add_executable(hft_latency_test
    test/test_latency.cpp
    src/MarketData.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/OrderManager.cpp
    src/TradeLogger.cpp
)



