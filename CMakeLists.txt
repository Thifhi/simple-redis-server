cmake_minimum_required(VERSION 3.13)

project(Redis-Server)

find_path(TCLAP_INCLUDE_DIRS "tclap/Arg.h")
find_package(spdlog CONFIG REQUIRED)

add_subdirectory(lib/eventloop)

set(SOURCES
    src/server.cpp
    src/client_manager.cpp
    src/client.cpp
    src/client_connection.cpp
    src/command_processor.cpp
    src/resp_parser.cpp
    src/redis_storage.cpp
)
set(HEADERS
    src/utils.h
    src/client.h
    src/client_manager.h
    src/coroutine.h
    src/client_connection.h
    src/command_processor.h
    src/resp_parser.h
    src/resp_encoders.h
    src/redis_storage.h
)
add_executable(server ${SOURCES} ${HEADERS})

target_compile_features(server PUBLIC cxx_std_20)
target_compile_options(server PUBLIC -fcoroutines)

# Optimization flag is non-optional, otherwise coroutines will eventually run out of stack memory. See:
# https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100897
target_compile_options(server PRIVATE -O2)

target_compile_options(server PRIVATE -Wall)

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/src ${TCLAP_INCLUDE_DIRS})

target_link_libraries(server PRIVATE spdlog::spdlog)
target_link_libraries(server PRIVATE eventloop)
