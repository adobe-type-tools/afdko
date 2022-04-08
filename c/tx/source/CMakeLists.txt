# find_package(LibXml2 REQUIRED)

add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(tx PUBLIC tx_shared libxml2_static)

message(STATUS "tx/source: LIBXML2_INCLUDE DIRS: ${libxml2_static}")

install(TARGETS tx DESTINATION bin)
