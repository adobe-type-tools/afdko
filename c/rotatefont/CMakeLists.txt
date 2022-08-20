add_library(rotatefont OBJECT
    rotateFont.cpp
    rf_help.h
    rf_options.h
    rf_usage.h
)

target_include_directories(rotatefont PRIVATE ../shared/include ../shared/resource ../shared/include/tx_help)

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(rotatefont ${LIBXML2_TARGET})
endif()
