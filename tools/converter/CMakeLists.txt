project(Converter)

include(Progression)

add_executable(converter main.cpp)

SET_TARGET_POSTFIX( converter )

target_link_libraries(converter ${PROGRESSION_LIBS})
