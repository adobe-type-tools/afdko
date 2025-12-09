add_library(makeotf_pstoken STATIC pstoken.c)

target_link_libraries(makeotf_pstoken PUBLIC LibXml2::LibXml2)

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)
