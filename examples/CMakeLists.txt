cmake_minimum_required (VERSION 3.0)

add_executable(example1 example1.cpp)
add_executable(example2 example2.cpp)
add_executable(example3 example3.cpp)

set_target_properties(
    example1 example2 example3
    PROPERTIES
    DEBUG_POSTFIX _debug
    )

target_link_libraries(example1 ${EXTERNAL_LIBS} debug ${OGL_STARTER_DEBUG_LIB} optimized ${OGL_STARTER_LIB})
target_link_libraries(example2 ${EXTERNAL_LIBS} debug ${OGL_STARTER_DEBUG_LIB} optimized ${OGL_STARTER_LIB})
target_link_libraries(example3 ${EXTERNAL_LIBS} debug ${OGL_STARTER_DEBUG_LIB} optimized ${OGL_STARTER_LIB})
