add_library(makeotf_cffread STATIC cffread.c)

target_include_directories(makeotf_cffread PRIVATE AFTER $<$<COMPILE_LANGUAGE:CXX>:${ANTLR4_INCLUDE_DIRS}>)

target_link_libraries(makeotf_cffread PUBLIC ${CHOSEN_LIBXML2_LIBRARY})

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(makeotf_cffread ${LIBXML2_TARGET})
endif()

target_link_libraries(makeotf_cffread PUBLIC afdko::antlr4)

target_compile_definitions(makeotf_cffread PRIVATE $<$<CONFIG:Debug>:CFF_DEBUG=1> CFF_T13_SUPPORT=0)
