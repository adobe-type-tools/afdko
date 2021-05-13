add_executable(rotatefont
    rotateFont.c
    help.h
    options.h
    usage.h
)

target_include_directories(rotatefont PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(rotatefont PUBLIC tx_shared)
install(TARGETS rotatefont DESTINATION bin)
