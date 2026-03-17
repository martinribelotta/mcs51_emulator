cmake_minimum_required(VERSION 3.16)

project(mcs51_emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(lib)

add_executable(mcs51_emulator
    src/main.c
    src/hex_loader.c
)

target_link_libraries(mcs51_emulator PRIVATE mcs51_lib)

target_compile_options(mcs51_emulator PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

enable_testing()

add_executable(mcs51_tests
    tests/test_runner.c
)

target_link_libraries(mcs51_tests PRIVATE mcs51_lib)

install(EXPORT mcs51Targets
    FILE mcs51Targets.cmake
    NAMESPACE mcs51::
    DESTINATION lib/cmake/mcs51
)

target_compile_options(mcs51_tests PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_test(NAME mcs51_tests COMMAND mcs51_tests)
