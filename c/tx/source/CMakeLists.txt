find_package(LibXml2 REQUIRED)

add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(tx PUBLIC tx_shared ${LIBXML2_LIBRARY})


install(TARGETS tx DESTINATION bin)
