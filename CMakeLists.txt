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

# Boost.Asio, Threads
find_package(Boost 1.91 REQUIRED CONFIG)
find_package(Threads REQUIRED)

# the engine library; consumers link this to get the code + include/ on their path
add_library(celebrant STATIC src/order_book.cpp src/codec.cpp src/engine.cpp src/engine_runner.cpp src/gateway.cpp src/connection.cpp)
target_include_directories(celebrant PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(celebrant PUBLIC Boost::headers Threads::Threads)

add_executable(celebrant_server src/main.cpp)
target_link_libraries(celebrant_server PRIVATE celebrant)

enable_testing()
add_subdirectory(test)
add_subdirectory(bench)
