include( Progression )

add_executable( sceneViewer sceneViewer.cpp)
#add_executable( luaExample luaExample.cpp )

set_target_properties(
    sceneViewer #luaExample
    PROPERTIES
    DEBUG_POSTFIX _debug
)

set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT sceneViewer )

target_link_libraries( sceneViewer ${PROGRESSION_LIBS} )
#target_link_libraries(luaExample ${PROGRESSION_LIBS})
