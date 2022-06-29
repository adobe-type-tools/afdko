add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(tx PUBLIC tx_shared)

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(tx PUBLIC ${LIBXML2_LIBRARY})
else()
    add_dependencies(tx ${LIBXML2_TARGET})
    target_compile_definitions(tx PUBLIC -DLIBXML_STATIC)
    if (WIN32)
       target_link_libraries(tx PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
       target_link_libraries(tx PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

install(TARGETS tx DESTINATION bin)
