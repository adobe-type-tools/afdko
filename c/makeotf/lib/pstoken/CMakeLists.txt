add_library(makeotf_pstoken STATIC pstoken.c)

target_link_libraries(makeotf_pstoken PUBLIC ${CHOSEN_LIBXML2_LIBRARY})

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(makeotf_pstoken ${LIBXML2_TARGET})
endif()

target_compile_definitions(makeotf_pstoken PRIVATE PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)
