project(auto_add_image)

include(Progression)

add_executable(auto_add_image main.cpp)

SET_TARGET_POSTFIX( auto_add_image )

target_link_libraries(auto_add_image ${PROGRESSION_LIBS})
