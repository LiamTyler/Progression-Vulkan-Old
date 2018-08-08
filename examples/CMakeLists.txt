include(Progression)

add_executable(example1 example1.cpp)
add_executable(example2 example2.cpp)
add_executable(example3 example3.cpp)

set_target_properties(
    example1 example2 example3
    PROPERTIES
    DEBUG_POSTFIX _debug
    )

#target_link_libraries(example1 ${EXTERNAL_LIBS} debug ${PROGRESSION_LIB_NAME_DEBUG} optimized ${PROGRESSION_LIB_NAME})
#target_link_libraries(example2 ${EXTERNAL_LIBS} debug ${PROGRESSION_LIB_NAME_DEBUG} optimized ${PROGRESSION_LIB_NAME})
#target_link_libraries(example3 ${EXTERNAL_LIBS} debug ${PROGRESSION_LIB_NAME_DEBUG} optimized ${PROGRESSION_LIB_NAME})
target_link_libraries(example1 ${PROGRESSION_LIBS})
target_link_libraries(example2 ${PROGRESSION_LIBS})
target_link_libraries(example3 ${PROGRESSION_LIBS})
