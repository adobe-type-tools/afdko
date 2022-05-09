add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)

target_link_libraries(rotatefont PUBLIC tx_shared)

if(WIN32)
  target_compile_definitions(rotatefont PUBLIC -DLIBXML_STATIC)
  target_link_libraries(rotatefont PUBLIC wsock32 ws2_32 libxml2)
else()
  target_link_libraries(rotatefont PUBLIC ${LIBXML2_LIBRARY})
endif()

install(TARGETS rotatefont DESTINATION bin)
