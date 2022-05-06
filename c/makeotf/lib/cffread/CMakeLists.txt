add_library(makeotf_cffread STATIC cffread.c)

target_include_directories(makeotf_cffread PRIVATE AFTER $<$<COMPILE_LANGUAGE:CXX>:${ANTLR4_INCLUDE_DIRS}> ${LIBXML2_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR_2})
target_link_libraries(makeotf_cffread PUBLIC libxml2 antlr4_static)

target_compile_definitions(makeotf_cffread PRIVATE $<$<CONFIG:Debug>:CFF_DEBUG=1> CFF_T13_SUPPORT=0)