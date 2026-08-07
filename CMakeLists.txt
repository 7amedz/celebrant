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

# --- SBE codec generation ---
find_package(Java REQUIRED COMPONENTS Runtime)

set(SBE_JAR     ${CMAKE_CURRENT_SOURCE_DIR}/tools/sbe-all-1.39.0.jar)
set(SBE_SCHEMA  ${CMAKE_CURRENT_SOURCE_DIR}/schema/celebrant.xml)
set(SBE_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

set(SBE_GEN_HEADERS
    ${SBE_GEN_DIR}/celebrant_sbe/MessageHeader.h
    ${SBE_GEN_DIR}/celebrant_sbe/NewOrderSingle.h
    ${SBE_GEN_DIR}/celebrant_sbe/OrderCancelRequest.h
    ${SBE_GEN_DIR}/celebrant_sbe/ExecutionReport.h
    ${SBE_GEN_DIR}/celebrant_sbe/Side.h
    ${SBE_GEN_DIR}/celebrant_sbe/OrdType.h
    ${SBE_GEN_DIR}/celebrant_sbe/ExecType.h
    ${SBE_GEN_DIR}/celebrant_sbe/OrdStatus.h
    ${SBE_GEN_DIR}/celebrant_sbe/OrdRejReason.h)

add_custom_command(
    OUTPUT ${SBE_GEN_HEADERS}
    COMMAND ${Java_JAVA_EXECUTABLE}
            -Dsbe.output.dir=${SBE_GEN_DIR}
            -Dsbe.target.language=Cpp
            -jar ${SBE_JAR}
            ${SBE_SCHEMA}
    DEPENDS ${SBE_SCHEMA} ${SBE_JAR}
    COMMENT "Generating SBE codec from schema/celebrant.xml"
    VERBATIM)

add_custom_target(sbe_codegen DEPENDS ${SBE_GEN_HEADERS})

# the engine library; consumers link this to get the code + include/ on their path
add_library(celebrant STATIC src/order_book.cpp src/codec.cpp src/sbe_codec.cpp src/engine.cpp src/engine_runner.cpp src/gateway.cpp src/connection.cpp)
target_include_directories(celebrant PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# generated SBE headers: built before celebrant compiles, put on private include path
add_dependencies(celebrant sbe_codegen)
target_include_directories(celebrant PRIVATE ${SBE_GEN_DIR})
target_link_libraries(celebrant PUBLIC Boost::headers Threads::Threads)

add_executable(celebrant_server src/main.cpp)
target_link_libraries(celebrant_server PRIVATE celebrant)

enable_testing()
add_subdirectory(test)
add_subdirectory(bench)
