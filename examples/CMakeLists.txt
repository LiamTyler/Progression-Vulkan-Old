include(Progression)

add_executable(example example.cpp)
add_executable(luaExample luaExample.cpp)

set_target_properties(
    example luaExample
    PROPERTIES
    DEBUG_POSTFIX _debug
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT example)

target_link_libraries(example ${PROGRESSION_LIBS})
target_link_libraries(luaExample ${PROGRESSION_LIBS})
