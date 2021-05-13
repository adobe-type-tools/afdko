add_library(makeotf_cffread STATIC cffread.c)

target_compile_options(makeotf_cffread PRIVATE $<$<CONFIG:Debug>:-DCFF_DEBUG=1> -DCFF_T13_SUPPORT=0)

