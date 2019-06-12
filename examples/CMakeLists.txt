include(Progression)

add_executable(example example.cpp)

set_target_properties(
    example
    PROPERTIES
    DEBUG_POSTFIX _debug
)

target_link_libraries(example ${PROGRESSION_LIBS})
