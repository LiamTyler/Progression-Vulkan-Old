set(PROGRESSION_INCLUDE_DIRS
    ${PROGRESSION_DIR}/ext
    ${PROGRESSION_DIR}/ext/glfw/include
    ${PROGRESSION_DIR}/include
    ${PROGRESSION_DIR}/project
    ${PROGRESSION_DIR}/project/src
	${PROGRESSION_DIR}/ext/glad/include
    ${PROGRESSION_DIR}/ext/nanogui/include
    )

set(PROGRESSION_LIBS
    glfw
    nanogui
    )

include_directories(${PROGRESSION_INCLUDE_DIRS})
