set(MCS51_UNIT_TESTS
    test_nop
    test_inc_dec_a
    test_add_addc_subb
    test_mov_basic
    test_mov_indirect_and_rn
    test_logic_ops
    test_jumps_basic
    test_bit_jumps
    test_djnz_cjne
    test_calls_jmp
    test_movc_movx
    test_movc_pc
    test_da_mul_div_xch
    test_inc_dec_misc
    test_parity_flag
    test_add_flags
    test_addc_subb_flags
    test_da_flags
    test_idle_wakeup
    test_timer0_counter_edges
    test_timer2_capture_mode
    test_timer2_external_reload_mode
    test_timer2_baud_profile_tf2
    test_timer2_interrupt_from_exf2
)

foreach(TEST_NAME IN LISTS MCS51_UNIT_TESTS)
    set(TEST_TARGET mcs51_${TEST_NAME})
    set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.c)

    add_executable(${TEST_TARGET}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test_common.c
        ${TEST_SOURCE}
    )

    target_link_libraries(${TEST_TARGET} PRIVATE mcs51_lib)
    target_compile_definitions(${TEST_TARGET} PRIVATE TEST_ENTRY=${TEST_NAME})
    target_compile_options(${TEST_TARGET} PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    )

    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
endforeach()
