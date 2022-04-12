add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared ${LIBXML2_INCLUDE_DIRS})
target_link_libraries(rotatefont PUBLIC tx_shared libxml2 ZLIB::ZLIB Iconv::Iconv LibLZMA::LibLZMA)
install(TARGETS rotatefont DESTINATION bin)
