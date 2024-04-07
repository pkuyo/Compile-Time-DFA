cmake_minimum_required(VERSION 3.27)
project(CompileTimeDfa)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-fconstexpr-loop-limit=2147483647)
add_compile_options(-fconstexpr-ops-limit=2147483647)
add_compile_options(-O2)
add_executable(CompileTimeDfa main.cpp
        ct_list.h
        ct_string.h
        ct_regex.h
        ct_utils.h)
