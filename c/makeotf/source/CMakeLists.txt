add_executable(makeotfexe main.cpp)

target_sources(makeotfexe PRIVATE
    cb.c
    cbpriv.c
    c_main.c
    fcdb.c
    file.c
    mac.c
    makeotflib.h
    cb.h
    cbpriv.h
    fcdb.h
    file.h
    lctype.h
    lerrno.h
    lstdio.h
    lstdlib.h
    lstring.h
    package.h
    systemspecific.h
)

target_link_libraries(makeotfexe PRIVATE ctutil dynarr hotconv makeotf_pstoken typecomp makeotf_cffread)

if (HAVE_M_LIB)
    target_link_libraries(makeotfexe PRIVATE m)
endif ()

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(makeotfexe PUBLIC ${LIBXML2_LIBRARY})
else ()
    if (WIN32)
       target_link_libraries(makeotfexe PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
       target_link_libraries(makeotfexe PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

target_compile_definitions(makeotfexe PRIVATE MAKEOTFLIB_EXPORTS=1)

# set_target_properties(makeotfexe PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS makeotfexe DESTINATION bin)
