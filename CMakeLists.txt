cmake_minimum_required(VERSION 3.5.0)
project(rvcc VERSION 0.1.0 LANGUAGES C)

add_executable(rvcc main.c)

target_compile_options(rvcc PRIVATE -std=c11 -g -fno-common)