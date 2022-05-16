add_library(makeotf_pstoken STATIC pstoken.c)

if(${LibXml2_FOUND})
    target_link_libraries(makeotf_pstoken PUBLIC ${LIBXML2_LIBRARY})
else()
    target_link_libraries(makeotf_pstoken PUBLIC ${LIBXML2_WIN_LIBRARIES})
endif()

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)
