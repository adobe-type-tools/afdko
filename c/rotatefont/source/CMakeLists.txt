add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared ${LIBXML2_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR_2})

if(WIN32)
  target_compile_definitions(rotatefont PUBLIC -DLIBXML_STATIC)
  target_link_libraries(rotatefont PUBLIC wsock32 ws2_32 libxml2 tx_shared)
else()
target_link_libraries(rotatefont PUBLIC libxml2 tx_shared)
endif()

target_link_libraries(rotatefont PUBLIC libxml2 tx_shared)
install(TARGETS rotatefont DESTINATION bin)