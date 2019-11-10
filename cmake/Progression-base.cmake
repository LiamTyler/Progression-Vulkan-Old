set(SYSTEM_LIBS "")
if (UNIX AND NOT APPLE)
    set(SYSTEM_LIBS
        dl
        stdc++fs
    )
endif()

function( SET_TARGET_POSTFIX )
    foreach(arg IN LISTS ARGN)
        set_target_properties(
            ${arg}
            PROPERTIES
            DEBUG_POSTFIX _debug
            RELEASE_POSTFIX _release
            SHIP_POSTFIX _ship
        )
    endforeach()
endfunction()

find_package(Vulkan REQUIRED)

set(PROGRESSION_LIBS
    glfw
    meshoptimizer
    lz4
    Vulkan::Vulkan
    lua
    assimp
    ${SYSTEM_LIBS}
)

set(PROGRESSION_INCLUDE_DIRS
    ${PROGRESSION_DIR}
    ${PROGRESSION_DIR}/ext
    ${PROGRESSION_DIR}/ext/lua
    ${PROGRESSION_DIR}/ext/getopt
    ${PROGRESSION_DIR}/ext/entt/src
    ${PROGRESSION_DIR}/ext/glfw/include
    ${PROGRESSION_DIR}/ext/assimp/include
    ${PROGRESSION_DIR}/progression
    ${Vulkan_INCLUDE_DIRS}
)

get_filename_component(VULKAN_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
set(PROGRESSION_LIB_DIR
    ${VULKAN_LIB_DIR}
)

if (PROGRESSION_AUDIO)
    set(PROGRESSION_LIBS ${PROGRESSION_LIBS} openal sndfile)
endif()

set_property(
    DIRECTORY
    PROPERTY
    COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:CMAKE_DEFINE_DEBUG_BUILD>
    $<$<CONFIG:Release>:CMAKE_DEFINE_RELEASE_BUILD>
    $<$<CONFIG:Ship>:CMAKE_DEFINE_SHIP_BUILD>
    PG_CPP_VERSION
    _CRT_SECURE_NO_WARNINGS
    #_ITERATOR_DEBUG_LEVEL=0
)

#if(MSVC)
#    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX /MT")
#    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /MT")
#    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /MT")
#    set(CMAKE_CXX_FLAGS_SHIP "${CMAKE_CXX_FLAGS_SHIP} /O2 /MT")
#endif()

link_directories(${PROGRESSION_LIB_DIR})
include_directories(${PROGRESSION_INCLUDE_DIRS})
