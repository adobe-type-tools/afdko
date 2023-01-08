add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(rotatefont PUBLIC tx_shared)

target_link_libraries(rotatefont PUBLIC ${CHOSEN_LIBXML2_LIBRARY})

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(rotatefont ${LIBXML2_TARGET})
    target_compile_definitions(rotatefont PUBLIC -DLIBXML_STATIC)
endif()

install(TARGETS rotatefont DESTINATION bin)
