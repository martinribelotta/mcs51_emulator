add_library(mcs51_lib
    cpu.c
    decoder.c
    instr_impl.c
    mem_map.c
    timers.c
    timing.c
    uart.c
)

target_include_directories(mcs51_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include/mcs51>
)

install(TARGETS mcs51_lib
    EXPORT mcs51Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    cpu.h
    decoder.h
    instr_impl.h
    instr_list.h
    mem_map.h
    timers.h
    timing.h
    uart.h
    DESTINATION include/mcs51
)
