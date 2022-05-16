add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(rotatefont PUBLIC tx_shared)

if(${LibXml2_FOUND})
    target_link_libraries(rotatefont PUBLIC ${LIBXML2_LIBRARY})
else()
    target_compile_definitions(rotatefont PUBLIC -DLIBXML_STATIC)
    target_link_libraries(rotatefont PUBLIC ${LIBXML2_WIN_LIBRARIES})
endif()

install(TARGETS rotatefont DESTINATION bin)
