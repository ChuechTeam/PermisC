cmake_minimum_required(VERSION 3.10)
project(PermisC C)

set(CMAKE_C_STANDARD 11)

add_executable(PermisC
        src/main.c
        src/route.c
        src/profile.c
        src/avl.c
        src/avl.h
        src/string_avl.c
        src/string_avl.h
)

# Stop Windows from complaining about """unsafe""" functions in stdlib
if (MSVC)
    target_compile_definitions(PermisC PUBLIC _CRT_SECURE_NO_WARNINGS=1)
    # Can be useful to see what assembly code is generated
    target_compile_options(PermisC PUBLIC /FAs)
endif()