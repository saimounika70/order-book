cmake_minimum_required(VERSION 3.16)
project(OrderBook CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release build = optimisations on
# Debug build = sanitisers on
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native)
else()
    add_compile_options(-O0 -g -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# shared sources
add_library(orderbook_lib
    src/order_book_map.cpp
    src/order_book_fast.cpp)

# tests
add_executable(tests tests/test_book.cpp)
target_link_libraries(tests orderbook_lib)

# benchmark
add_executable(bench bench/benchmark.cpp)
target_link_libraries(bench orderbook_lib)