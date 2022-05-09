add_executable(tx
    tx.c
    help.h
    options.h
    usage.h
)

target_include_directories(tx PRIVATE ../../shared/include ../../shared/resource ../../shared/source/tx_shared)
target_link_libraries(tx PUBLIC tx_shared)

if(WIN32)
  target_compile_definitions(tx PUBLIC -DLIBXML_STATIC)
  target_link_libraries(tx PUBLIC wsock32 ws2_32 libxml2)
else()
  target_link_libraries(tx PUBLIC ${LIBXML2_LIBRARY})
endif()

install(TARGETS tx DESTINATION bin)
