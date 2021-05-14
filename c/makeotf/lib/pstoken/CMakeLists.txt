add_library(makeotf_pstoken STATIC pstoken.c)

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)
