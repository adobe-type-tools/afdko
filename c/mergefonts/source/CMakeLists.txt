add_executable(mergefonts
    mergeFonts.c
    help.h
    options.h
    usage.h
)

target_include_directories(mergefonts PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(mergefonts PUBLIC tx_shared)

if(WIN32)
  target_compile_definitions(mergefonts PUBLIC -DLIBXML_STATIC)
  target_link_libraries(mergefonts PUBLIC wsock32 ws2_32 libxml2 tx_shared)
else()
  target_link_libraries(mergefonts PUBLIC ${LIBXML2_LIBRARY})
endif()

install(TARGETS mergefonts DESTINATION bin)
