include(Progression)

add_executable(objToPGModel objToPGModel.cpp)
add_executable(objToRTModel objToRTModel.cpp)

set_target_properties(
    objToPGModel objToRTModel
    PROPERTIES
    DEBUG_POSTFIX _debug
    )

target_link_libraries(objToPGModel ${PROGRESSION_LIBS})
target_link_libraries(objToRTModel ${PROGRESSION_LIBS})
