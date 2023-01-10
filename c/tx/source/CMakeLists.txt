add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(tx PUBLIC tx_shared)

target_link_libraries(tx PUBLIC ${CHOSEN_LIBXML2_LIBRARY})
if (${NEED_LIBXML2_DEPEND})
    add_dependencies(tx ${LIBXML2_TARGET})
    target_compile_definitions(tx PUBLIC -DLIBXML_STATIC)
endif()

install(TARGETS tx DESTINATION bin)
