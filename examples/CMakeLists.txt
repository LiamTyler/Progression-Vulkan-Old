include(Progression)

add_executable(example1 example1.cpp)
add_executable(example2 example2.cpp)
add_executable(example3 example3.cpp)
add_executable(forward forward.cpp)
add_executable(deferred deferred.cpp)
add_executable(deferred_light_vols deferred_light_vols.cpp)
add_executable(tiled tiled.cpp)
add_executable(demo demo.cpp)
add_executable(shadow shadow.cpp)

set_target_properties(
    example1 example2 example3 forward deferred deferred_light_vols tiled demo shadow
    PROPERTIES
    DEBUG_POSTFIX _debug
)

target_link_libraries(example1 ${PROGRESSION_LIBS})
target_link_libraries(example2 ${PROGRESSION_LIBS})
target_link_libraries(example3 ${PROGRESSION_LIBS})
target_link_libraries(forward ${PROGRESSION_LIBS})
target_link_libraries(deferred ${PROGRESSION_LIBS})
target_link_libraries(deferred_light_vols ${PROGRESSION_LIBS})
target_link_libraries(tiled ${PROGRESSION_LIBS})
target_link_libraries(demo ${PROGRESSION_LIBS})
target_link_libraries(shadow ${PROGRESSION_LIBS})

if (PROGRESSION_AUDIO)
    add_executable(audio audio_example.cpp)
    set_target_properties(
        audio
        PROPERTIES
        DEBUG_POSTFIX _debug
    )
    target_link_libraries(audio ${PROGRESSION_LIBS})
endif()
