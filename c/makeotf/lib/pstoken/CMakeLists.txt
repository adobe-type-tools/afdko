add_library(makeotf_pstoken STATIC pstoken.c)

if(WIN32)
    target_link_libraries(makeotf_pstoken PUBLIC wsock32 ws2_32 libxml2)
else()
    target_link_libraries(makeotf_pstoken PUBLIC ${LIBXML2_LIBRARY})
endif()

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)
