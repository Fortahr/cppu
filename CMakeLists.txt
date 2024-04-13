cmake_minimum_required(VERSION 3.8)

set(CPPU_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE PATH "")

project(cppu)
add_library(cppu INTERFACE)
target_include_directories(cppu INTERFACE ${CPPU_INCLUDE_DIR})

mark_as_advanced(CPPU_INCLUDE_DIR)