set(PROGRESSION_INCLUDE_DIRS
    ${PROGRESSION_DIR}
    ${PROGRESSION_DIR}/ext
    ${PROGRESSION_DIR}/ext/getopt
    ${PROGRESSION_DIR}/ext/glfw/include
    ${PROGRESSION_DIR}/progression
    ${PROGRESSION_DIR}/progression/src
)

if (MSVC)
    set(PROGRESSION_INCLUDE_DIRS
        C:/VulkanSDK/1.1.101.0/Include
        ${PROGRESSION_INCLUDE_DIRS}
    )
endif()

set(SYSTEM_LIBS "")
if (UNIX AND NOT APPLE)
    set(SYSTEM_LIBS
        dl
        stdc++fs
    )
endif()

set(PROGRESSION_LIBS
    glfw
    meshoptimizer
    lz4
    vulkan-1
    ${SYSTEM_LIBS}
)

if (PROGRESSION_AUDIO)
    set(PROGRESSION_LIBS ${PROGRESSION_LIBS} openal sndfile)
endif()

if (NOT PROGRESSION_LIB_DIR)
    set(PROGRESSION_LIB_DIR
        ${PROGRESSION_DIR}/build/lib
        C:/VulkanSDK/1.1.101.0/Lib
    )
endif()

if (NOT PROGRESSION_BIN_DIR)
    set(PROGRESSION_BIN_DIR ${PROGRESSION_DIR}/build/bin)
endif()

set_property(
    DIRECTORY
    PROPERTY
    COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:CMAKE_DEFINE_DEBUG_BUILD>
    $<$<CONFIG:Release>:CMAKE_DEFINE_RELEASE_BUILD>
    $<$<CONFIG:Ship>:CMAKE_DEFINE_SHIP_BUILD>
)

link_directories(${PROGRESSION_LIB_DIR})
include_directories(${PROGRESSION_INCLUDE_DIRS})
