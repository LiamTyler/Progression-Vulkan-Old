include(Progression-base)

if (NOT PROGRESSION_LIB_DIR)
    set(PROGRESSION_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT PROGRESSION_BIN_DIR)
    set(PROGRESSION_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
endif()

if (WIN32)
    set(SDL2_DLL ${SDL2_DIR}/lib/x64/SDL2.dll)
    file(COPY ${SDL2_DLL} DESTINATION ${PROGRESSION_BIN_DIR})
endif()

set(PROGRESSION_LIBS
    ${PROGRESSION_LIBS}
    progression
    )

link_directories(${PROGRESSION_LIB_DIR})
