set(PROGRESSION_INCLUDE_DIRS
    ${PROGRESSION_DIR}/ext
    ${PROGRESSION_DIR}/progression
    ${PROGRESSION_DIR}/progression/src
    ${PROGRESSION_DIR}/ext/nanogui/include
    ${NANOGUI_EXTRA_INCS}
    )

set(PROGRESSION_LIBS
    nanogui
    ${NANOGUI_EXTRA_LIBS}
)

include_directories(${PROGRESSION_INCLUDE_DIRS})
