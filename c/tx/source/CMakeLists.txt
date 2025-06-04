add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(tx PUBLIC tx_shared)

target_link_libraries(tx PUBLIC LibXml2::LibXml2)

install(TARGETS tx DESTINATION bin)
