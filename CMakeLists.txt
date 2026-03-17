cmake_minimum_required(VERSION 3.16)

project(mcs51_emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(mcs51_emulator
    cpu.c
    decoder.c
    instr_impl.c
    hex_loader.c
    mem_map.c
    timing.c
    uart.c
    main.c
)

target_include_directories(mcs51_emulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(mcs51_emulator PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

enable_testing()

add_executable(mcs51_tests
    cpu.c
    decoder.c
    instr_impl.c
    mem_map.c
    timing.c
    uart.c
    tests/test_runner.c
)

target_include_directories(mcs51_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(mcs51_tests PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_test(NAME mcs51_tests COMMAND mcs51_tests)
