cmake_minimum_required(VERSION 2.8.11)

project(B)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c89 -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -pedantic -pedantic-errors -Wunreachable-code -Wformat=2 -Wno-format-nonliteral -fno-builtin -Werror -g -D_GNU_SOURCE")

add_library(b STATIC b.c)
add_executable(bc main.c)
target_link_libraries(bc b)
