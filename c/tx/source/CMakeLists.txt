add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared ${LIBXML2_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR_2})
target_link_libraries(tx PUBLIC libxml2 tx_shared)

if(WIN32)
  target_compile_definitions(tx PUBLIC -DLIBXML_STATIC)
  target_link_libraries(tx PUBLIC wsock32 ws2_32 libxml2)
else()
  target_link_libraries(tx PUBLIC libxml2)
endif()

install(TARGETS tx DESTINATION bin)