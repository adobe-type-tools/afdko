add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(tx PUBLIC tx_shared)

if(${LibXml2_FOUND})
    target_link_libraries(tx PUBLIC ${LIBXML2_LIBRARY})
else()
    target_compile_definitions(tx PUBLIC -DLIBXML_STATIC)
    target_link_libraries(tx PUBLIC ${LIBXML2_WIN_LIBRARIES})
endif()

install(TARGETS tx DESTINATION bin)
