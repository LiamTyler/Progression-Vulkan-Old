include( Progression )

add_executable( sceneViewer sceneViewer.cpp)
add_executable( image_compressor image_compressor.cpp)
#add_executable( luaExample luaExample.cpp )

set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT sceneViewer )
SET_TARGET_POSTFIX( sceneViewer )

target_link_libraries( sceneViewer ${PROGRESSION_LIBS} )
target_link_libraries( image_compressor ${PROGRESSION_LIBS} )
