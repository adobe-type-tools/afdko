find_package(LibXml2 REQUIRED)

add_executable(mergefonts
    mergeFonts.c
    help.h
    options.h
    usage.h
)

target_include_directories(mergefonts PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(mergefonts PUBLIC tx_shared ${LIBXML2_LIBRARY})
install(TARGETS mergefonts DESTINATION bin)

