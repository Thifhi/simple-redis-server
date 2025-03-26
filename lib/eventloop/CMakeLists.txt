add_compile_options(-fcoroutines)

add_library(eventloop STATIC src/loop.cpp include/loop.h)

target_compile_features(eventloop PUBLIC cxx_std_20)

target_include_directories(eventloop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
