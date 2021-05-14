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
