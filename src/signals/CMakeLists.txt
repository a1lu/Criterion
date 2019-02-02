
cmake_minimum_required (VERSION 2.8)

find_program(SHELL sh)

if(SHELL)
    message(${SHELL})
add_custom_command (
    OUTPUT signal_names.c signal_names.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/create_source.sh
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/create_source.sh
    )
    set(C_SOURCE signal_names.c)
else()
    message("no shell")
    set(C_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/signal_names_default.c)
endif()


add_library(signalnames
    ${C_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/signal_names.h)

target_include_directories(signalnames PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(signalnames PRIVATE -fPIC)

add_custom_target(
    signal_sources signalnames
    DEPENDS signal_names.c signal_names.h
    )
