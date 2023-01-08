add_executable(mergefonts
    mergeFonts.c
    help.h
    options.h
    usage.h
)

target_include_directories(mergefonts PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(mergefonts PUBLIC tx_shared)
target_link_libraries(mergefonts PUBLIC ${CHOSEN_LIBXML2_LIBRARY})

if (${NEED_LIBXML_DEPEND})
    add_dependencies(mergefonts ${LIBXML2_TARGET})
    target_compile_definitions(mergefonts PUBLIC -DLIBXML_STATIC)
endif()

install(TARGETS mergefonts DESTINATION bin)
