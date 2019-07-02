include(Progression)

add_executable(createFastFile createFastFile.cpp)

set_target_properties(
    createFastFile
    PROPERTIES
    DEBUG_POSTFIX _debug
    )

target_link_libraries(createFastFile ${PROGRESSION_LIBS})
