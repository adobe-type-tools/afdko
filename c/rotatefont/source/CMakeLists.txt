add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(rotatefont PUBLIC tx_shared)

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(rotatefont PUBLIC ${LIBXML2_LIBRARY})
else()
    add_dependencies(rotatefont ${LIBXML2_TARGET})
    target_compile_definitions(rotatefont PUBLIC -DLIBXML_STATIC)
    if (WIN32)
        target_link_libraries(rotatefont PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
        target_link_libraries(rotatefont PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

install(TARGETS rotatefont DESTINATION bin)
