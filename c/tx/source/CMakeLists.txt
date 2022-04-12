add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(tx PUBLIC tx_shared libxml2 ZLIB::ZLIB Iconv::Iconv LibLZMA::LibLZMA)
install(TARGETS tx DESTINATION bin)
