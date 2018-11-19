include(Progression)

add_executable(objToPGModel objToPGModel.cpp)

set_target_properties(
    objToPGModel
    PROPERTIES
    DEBUG_POSTFIX _debug
    )

target_link_libraries(objToPGModel ${PROGRESSION_LIBS})