cmake_minimum_required(VERSION 3.13)

project(http-server)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(http-server ${SOURCE_FILES})

target_include_directories(http-server PRIVATE ${CMAKE_SOURCE_DIR}/headers)
