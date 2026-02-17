cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smartwatch_v1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FILE(GLOB root_sources src/*.c)
message(STATUS root_sources : ${root_sources})

include_directories(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(src/ui)
add_subdirectory(src/applications)
add_subdirectory(src/managers)
add_subdirectory(src/threads)

target_sources(app PRIVATE ${root_sources})
