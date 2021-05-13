add_library(makeotf_pstoken STATIC pstoken.c)

target_compile_options(makeotf_pstoken PRIVATE -DPSTOKEN_DEBUG=1 -DCFF_DEBUG=1 -DCFF_T13_SUPPORT=0)
