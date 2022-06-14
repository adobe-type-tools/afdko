add_library(makeotf_cffread STATIC cffread.c)
add_dependencies(makeotf_cffread ${LIBXML2_TARGET})

target_include_directories(makeotf_cffread PRIVATE AFTER $<$<COMPILE_LANGUAGE:CXX>:${ANTLR4_INCLUDE_DIRS}>)

if (${LibXml2_FOUND})
    target_link_libraries(makeotf_cffread PUBLIC ${LIBXML2_LIBRARY})
else ()
    if (WIN32)
       target_link_libraries(makeotf_cffread PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
       target_link_libraries(makeotf_cffread PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

target_link_libraries(makeotf_cffread PUBLIC antlr4_static)

target_compile_definitions(makeotf_cffread PRIVATE $<$<CONFIG:Debug>:CFF_DEBUG=1> CFF_T13_SUPPORT=0)
