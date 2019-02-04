include(Progression)

add_executable(example1 example1.cpp)
add_executable(forward forward.cpp)
add_executable(lightDemo lightDemo.cpp)

set_target_properties(
    example1 forward lightDemo
    PROPERTIES
    DEBUG_POSTFIX _debug
)

target_link_libraries(example1 ${PROGRESSION_LIBS})
target_link_libraries(forward ${PROGRESSION_LIBS})
target_link_libraries(lightDemo ${PROGRESSION_LIBS})
