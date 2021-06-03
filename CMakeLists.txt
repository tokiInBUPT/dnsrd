project( DNSRD )
cmake_minimum_required( VERSION 2.8.12 )
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
file(GLOB_RECURSE SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    DNSPacket.c DNSPacket.h
    idTransfer.c idTransfer.h
    loadConfig.c loadConfig.h
    yyjson.c yyjson.h
    cli.c cli.h
    loadCache.c loadCache.h
    lhm.c lhm.h
    handler.c
    handler.h
)
add_executable( ${PROJECT_NAME} main.c ${SRCS} )
target_link_libraries(${PROJECT_NAME} ws2_32)