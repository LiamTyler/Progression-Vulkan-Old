set(PROGRESSION_INCLUDE_DIRS
    ${PROGRESSION_DIR}/ext
    ${PROGRESSION_DIR}/progression
    ${PROGRESSION_DIR}/progression/src
    ${PROGRESSION_DIR}/ext/nanogui/include
    ${NANOGUI_EXTRA_INCS}
    )

set(SYSTEM_LIBS "")
if (UNIX AND NOT APPLE)
    set(SYSTEM_LIBS "stdc++fs")
endif()


set(PROGRESSION_LIBS
    nanogui
    ${SYSTEM_LIBS}
    ${NANOGUI_EXTRA_LIBS}
)

if (NOT PROGRESSION_LIB_DIR)
    set(PROGRESSION_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT PROGRESSION_BIN_DIR)
    set(PROGRESSION_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
endif()

link_directories(${PROGRESSION_LIB_DIR})
include_directories(${PROGRESSION_INCLUDE_DIRS})
