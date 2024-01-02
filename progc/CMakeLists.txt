cmake_minimum_required(VERSION 3.10)
project(PermisC C)

set(CMAKE_C_STANDARD 11)

add_executable(PermisC
        src/main.c
        src/route.c
        src/profile.c
        src/avl.c
        src/string_avl.c
        src/computations/computation_t.c
        src/computations/computation_s.c
        src/computations/computation_d1.c
        src/options.c
)

target_include_directories(PermisC PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

# Stop Windows from complaining about """unsafe""" functions in stdlib
if (MSVC)
    target_compile_definitions(PermisC PUBLIC _CRT_SECURE_NO_WARNINGS=1)
    # Can be useful to see what assembly code is generated
    target_compile_options(PermisC PUBLIC /FAs)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_link_options(PermisC PUBLIC /LTCG)
    endif ()
endif ()

if (UNIX)
    target_compile_options(PermisC PUBLIC -Wall -Wno-unused-function)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(PermisC PUBLIC -flto)
    endif ()
endif ()