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

target_compile_definitions(absfont PRIVATE $<$<CONFIG:Debug>:ABF_DEBUG=1>)
target_compile_definitions(cfembed PRIVATE $<$<CONFIG:Debug>:CEF_DEBUG=1>)
target_compile_definitions(cffread PRIVATE $<$<CONFIG:Debug>:CFR_DEBUG=1>)
target_compile_definitions(cffwrite PRIVATE $<$<CONFIG:Debug>:CFW_DEBUG=1>)
target_compile_definitions(dynarr PRIVATE $<$<CONFIG:Debug>:DNA_DEBUG=1>)
target_compile_definitions(pstoken PRIVATE $<$<CONFIG:Debug>:PST_DEBUG=1>)
target_compile_definitions(pdfwrite PRIVATE $<$<CONFIG:Debug>:PDW_DEBUG=1>)
target_compile_definitions(t1cstr PRIVATE $<$<CONFIG:Debug>:T1C_DEBUG=1>)
target_compile_definitions(t2cstr PRIVATE $<$<CONFIG:Debug>:T1C_DEBUG=1>)
target_compile_definitions(t1read PRIVATE $<$<CONFIG:Debug>:T1R_DEBUG=1>)
target_compile_definitions(t1write PRIVATE $<$<CONFIG:Debug>:T1W_DEBUG=1>)
target_compile_definitions(ttread PRIVATE $<$<CONFIG:Debug>:TTR_DEBUG=1>)
target_compile_definitions(tx_shared PRIVATE $<$<CONFIG:Debug>:CFW_DEBUG=1>)

target_link_libraries(tx_shared PUBLIC LibXml2::LibXml2)

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
)

if(HAVE_M_LIB)
    target_link_libraries(tx_shared PUBLIC m)
endif()
