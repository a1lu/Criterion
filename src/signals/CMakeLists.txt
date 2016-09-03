INCLUDE (CheckIncludeFiles)
set (cmd "signals.sh")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set (header bits/signum.h)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
    set (header iso/signal_iso.h)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set (header sys/signal.h)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set (header sys/signal.h)
endif()

CHECK_INCLUDE_FILES(${header} HAVE_SIGNUM)

if(${HAVE_SIGNUM})
    find_file(signal_header ${header})
    message(${signal_header})
    message(${CMAKE_CURRENT_SOURCE_DIR})
    message(${PROJECT_SOURCE_DIR})
    add_custom_command (
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/signal_names.c
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${cmd} ${cmd}
        COMMAND /bin/bash ./${cmd} ${signal_header}
        #COMMAND /bin/bash ./${cmd} ${signal_header} ${PROJECT_SOURCE_DIR}/src/signals
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Creates/updates header SVN Revision Header"
        )
endif()

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_C_FLAGS "-fPIC")
add_library( SigNames signal_names.c)


#install (TARGETS SigNames DESTINATION bin)
#install (FILES SigNames.h DESTINATION include)
