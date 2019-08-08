project(Converter)

include(Progression)

add_executable(converter main.cpp)

set_target_properties(
    converter
    PROPERTIES
    DEBUG_POSTFIX _debug
)

target_link_libraries(converter ${PROGRESSION_LIBS})
