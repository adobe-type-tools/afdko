add_library(makeotf_cffread STATIC cffread.c)

target_compile_definitions(makeotf_cffread PRIVATE $<$<CONFIG:Debug>:CFF_DEBUG=1> CFF_T13_SUPPORT=0)

