include_directories(../include ../resource)

MACRO(ADD_AFDKOLIB lm)
    file(GLOB sl_sources "${lm}/*")
    add_library(${lm} STATIC ${sl_sources})
ENDMACRO(ADD_AFDKOLIB)

ADD_AFDKOLIB(absfont)
ADD_AFDKOLIB(cfembed)
ADD_AFDKOLIB(cffread)
ADD_AFDKOLIB(cffwrite)
ADD_AFDKOLIB(ctutil)
ADD_AFDKOLIB(dynarr)
ADD_AFDKOLIB(nameread)
ADD_AFDKOLIB(pdfwrite)
ADD_AFDKOLIB(pstoken)
ADD_AFDKOLIB(sfntread)
ADD_AFDKOLIB(sfntwrite)
ADD_AFDKOLIB(sha1)
ADD_AFDKOLIB(support)
ADD_AFDKOLIB(svgwrite)
ADD_AFDKOLIB(svread)
ADD_AFDKOLIB(t1cstr)
ADD_AFDKOLIB(t1read)
ADD_AFDKOLIB(t1write)
ADD_AFDKOLIB(t2cstr)
ADD_AFDKOLIB(ttread)
ADD_AFDKOLIB(tx_shared)
ADD_AFDKOLIB(uforead)
ADD_AFDKOLIB(ufowrite)
ADD_AFDKOLIB(varread)

target_compile_options(absfont PRIVATE $<$<CONFIG:Debug>:-DABF_DEBUG=1>)
target_compile_options(cfembed PRIVATE $<$<CONFIG:Debug>:-DCEF_DEBUG=1>)
target_compile_options(cffread PRIVATE $<$<CONFIG:Debug>:-DCFR_DEBUG=1>)
target_compile_options(cffwrite PRIVATE $<$<CONFIG:Debug>:-DCFW_DEBUG=1>)
target_compile_options(dynarr PRIVATE $<$<CONFIG:Debug>:-DDNA_DEBUG=1>)
target_compile_options(pstoken PRIVATE $<$<CONFIG:Debug>:-DPST_DEBUG=1>)
target_compile_options(pdfwrite PRIVATE $<$<CONFIG:Debug>:-DPDW_DEBUG=1>)
target_compile_options(t1cstr PRIVATE $<$<CONFIG:Debug>:-DT1C_DEBUG=1>)
target_compile_options(t2cstr PRIVATE $<$<CONFIG:Debug>:-DT1C_DEBUG=1>)
target_compile_options(t1read PRIVATE $<$<CONFIG:Debug>:-DT1R_DEBUG=1>)
target_compile_options(t1write PRIVATE $<$<CONFIG:Debug>:-DT1W_DEBUG=1>)
target_compile_options(ttread PRIVATE $<$<CONFIG:Debug>:-DTTR_DEBUG=1>)
target_compile_options(tx_shared PRIVATE $<$<CONFIG:Debug>:-DCFW_DEBUG=1>)

if(MSVC)
    target_compile_options(support PRIVATE -DCTL_CDECL=__cdecl -DCDECL=__cdecl
        -DOS=os_windowsNT -DISP=isp_i80486 -D_CRT_SECURE_NO_DEPRECATE
        $<$<CONFIG:Release>:-DNDEBUG>)
endif()

# These libraries are partially ordered

target_link_libraries(tx_shared PUBLIC
    cfembed
    cffread
    cffwrite
    ctutil
    pdfwrite
    sfntread
    sfntwrite
    nameread
    sha1
    support
    svgwrite
    svread
    t1read
    t1write
    t2cstr
    ttread
    uforead
    ufowrite
    pstoken
    t1cstr
    absfont
    dynarr
    varread
    m
)
