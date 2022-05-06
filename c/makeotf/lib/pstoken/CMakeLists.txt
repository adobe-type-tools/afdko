add_library(makeotf_pstoken STATIC pstoken.c)

target_include_directories(makeotf_pstoken PRIVATE ${LIBXML2_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR_2})
target_link_libraries(makeotf_pstoken PUBLIC libxml2)

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)