add_library(makeotf_pstoken STATIC pstoken.c)

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(makeotf_pstoken PUBLIC ${LIBXML2_LIBRARY})
else ()
    add_dependencies(makeotf_pstoken ${LIBXML2_TARGET})
    if (WIN32)
       target_link_libraries(makeotf_pstoken PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
       target_link_libraries(makeotf_pstoken PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)
