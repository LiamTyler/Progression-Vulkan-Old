set(PROGRESSION_INCLUDE_DIRS
    ${PROGRESSION_DIR}/ext
    ${PROGRESSION_DIR}/progression
    ${PROGRESSION_DIR}/progression/src
    ${PROGRESSION_DIR}/ext/nanogui/include
    ${NANOGUI_EXTRA_INCS}
    ${PROGRESSION_DIR}/ext/yaml-cpp/include
    )

set(PROGRESSION_LIBS
    yaml-cpp
    nanogui
    ${NANOGUI_EXTRA_LIBS}
)

include_directories(${PROGRESSION_INCLUDE_DIRS})
