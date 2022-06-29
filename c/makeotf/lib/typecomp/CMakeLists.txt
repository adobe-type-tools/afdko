add_library(typecomp STATIC
    cdv1axis.h
    charset.c
    charset.h
    common.h
    cs.c
    cs.h
    dict.c
    dict.h
    encoding.c
    encoding.h
    eurosans.h
    euroserf.h
    fdselect.c
    fdselect.h
    fillinmm_sans.h
    fillinmm_serif.h
    font.h
    igara0.h
    igara1.h
    igara2.h
    igara3.h
    igara4.h
    isocenm.h
    jenson.h
    jimbo0.h
    jimbo1.h
    jimbo2.h
    kepler.h
    package.h
    parse.c
    parse.h
    recode.c
    recode.h
    rgara0.h
    rgara1.h
    rgara2.h
    rgara3.h
    sindex.c
    sindex.h
    subr.c
    subr.h
    t13.c
    t13.h
    tc.c
)

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(typecomp PUBLIC ${LIBXML2_LIBRARY})
else ()
    add_dependencies(typecomp ${LIBXML2_TARGET})
    if (WIN32)
       target_link_libraries(typecomp PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
       target_link_libraries(typecomp PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

target_compile_definitions(typecomp PRIVATE
    TC_HINT_CHECK=1
    TC_T13_SUPPORT=0
    TC_EURO_SUPPORT=1
    TC_SUBR_SUPPORT=1
    TC_DEBUG=1
)
