set(RO_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
set(NRF_DIR RO_DIR)

if(NOT BOARD)
        set(BOARD $ENV{BOARD})
endif()

# Check if selected board is supported.
if(DEFINED RO_SUPPORTED_BOARDS)
        if(NOT BOARD IN_LIST RO_SUPPORTED_BOARDS)
                message(FATAL_ERROR "board ${BOARD} is not supported")
        endif()
endif()

# Check if selected build type is supported.
if(DEFINED RO_SUPPORTED_BUILD_TYPES)
        if(NOT CMAKE_BUILD_TYPE IN_LIST RO_SUPPORTED_BUILD_TYPES)
                message(FATAL_ERROR "${CMAKE_BUILD_TYPE} variant is not supported")
        endif()
endif()

# Add RO_DIR as a BOARD_ROOT in case the board is in RO_DIR
list(APPEND BOARD_ROOT ${RO_DIR})

# Add RO_DIR as a DTS_ROOT in case the dts binding is in RO_DIR
list(APPEND DTS_ROOT ${RO_DIR})
