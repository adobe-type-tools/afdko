add_executable(mergefonts
    mergeFonts.c
    help.h
    options.h
    usage.h
)

target_include_directories(mergefonts PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(mergefonts PUBLIC tx_shared)

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(mergefonts PUBLIC ${LIBXML2_LIBRARY})
else ()
    add_dependencies(mergefonts ${LIBXML2_TARGET})
    target_compile_definitions(mergefonts PUBLIC -DLIBXML_STATIC)
    if (WIN32)
        target_link_libraries(mergefonts PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
        target_link_libraries(mergefonts PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

install(TARGETS mergefonts DESTINATION bin)
