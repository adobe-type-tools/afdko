add_executable(makeotfexe main.cpp)

target_include_directories(makeotfexe PRIVATE ${LIBXML2_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR_2})

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

target_link_libraries(makeotfexe PRIVATE libxml2 ctutil dynarr hotconv makeotf_pstoken typecomp makeotf_cffread)

if(HAVE_M_LIB)
    target_link_libraries(makeotfexe PRIVATE m)
endif()

target_compile_definitions(makeotfexe PRIVATE MAKEOTFLIB_EXPORTS=1)

# set_target_properties(makeotfexe PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS makeotfexe DESTINATION bin)