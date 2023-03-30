cmake_minimum_required(VERSION 3.5.1)
project(parallel_scheduler)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -fPIC")
enable_testing()

set(CMAKE_BUILD_TYPE Debug)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_options(-g -pthread -fPIC -DDebug)
else()
    add_compile_options(-pthread -fPIC)
endif()
include_directories(./src/include)
add_library(task OBJECT src/task.c)
add_library(utils OBJECT src/utils.c)
add_library(error OBJECT src/error.c)

add_library(threads OBJECT src/PS_threads.c)

add_library(scheduler OBJECT src/scheduler.c)

add_library(ps SHARED)
target_sources(ps PUBLIC $<TARGET_OBJECTS:task> $<TARGET_OBJECTS:utils> $<TARGET_OBJECTS:error> $<TARGET_OBJECTS:threads> $<TARGET_OBJECTS:scheduler>)


add_executable(static_test test/static_test.c)
target_link_libraries(static_test ps)

add_test(NAME static_test COMMAND static_test)

add_executable(dynamic_test test/dynamic_test.c)
target_link_libraries(dynamic_test ps)

add_test(NAME dynamic_test COMMAND dynamic_test)

add_executable(guided_test test/guided_test.c)
target_link_libraries(guided_test ps)

add_test(NAME guided_test COMMAND guided_test)

add_custom_target(benchmark COMMAND guided_test COMMAND dynamic_test COMMAND static_test)
add_executable(main main.cpp)

add_executable(decay_test test/decay_test.c)
target_link_libraries(decay_test ps)
add_test(NAME decay_test COMMAND decay_test)
