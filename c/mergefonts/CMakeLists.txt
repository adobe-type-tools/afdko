add_library(mergefonts OBJECT
    mergeFonts.cpp
    mf_help.h
    mf_options.h
    mf_usage.h
)

target_include_directories(mergefonts PRIVATE ../shared/include ../shared/resource ../shared/include/tx_help)

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(mergefonts ${LIBXML2_TARGET})
endif()
