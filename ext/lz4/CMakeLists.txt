project(LZ4)

set(
    SRCS
    lz4.c 
    lz4hc.c 
    lz4frame.c 
    xxhash.c 
)

set(
    HEADERS
    lz4.h 
    lz4hc.h 
    lz4frame.h 
    xxhash.h 
)

if(MSVC)
    target_compile_options(progression PRIVATE "/MP")
endif()

add_library(lz4 STATIC ${SRCS} ${HEADERS})
