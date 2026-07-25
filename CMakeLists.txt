cmake_minimum_required(VERSION 3.21)

project(celebrant
    VERSION 0.1.0
    DESCRIPTION "A limit order matching engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile_commands.json for clangd / clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# the engine library; consumers link this to get the code + include/ on their path
add_library(celebrant STATIC src/order_book.cpp src/codec.cpp)
target_include_directories(celebrant PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()
add_subdirectory(test)
add_subdirectory(bench)
