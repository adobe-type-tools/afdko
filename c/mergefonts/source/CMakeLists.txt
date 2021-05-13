add_executable(mergefonts
    mergeFonts.c
    help.h
    options.h
    usage.h
)

target_include_directories(mergefonts PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(mergefonts PUBLIC tx_shared)
install(TARGETS mergefonts DESTINATION bin)

