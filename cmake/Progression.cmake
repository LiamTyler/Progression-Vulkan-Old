include(Progression-base)

if (NOT PROGRESSION_LIB_DIR)
    set(PROGRESSION_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT PROGRESSION_BIN_DIR)
    set(PROGRESSION_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
endif()

set(PROGRESSION_LIBS
    ${PROGRESSION_LIBS}
    progression
)

link_directories(${PROGRESSION_LIB_DIR})
