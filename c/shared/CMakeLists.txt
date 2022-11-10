add_library(shared OBJECT
    absfont/absfont_afm.cpp
    absfont/absfont.cpp
    absfont/absfont_compare.cpp
    absfont/absfont_desc.cpp
    absfont/absfont_draw.cpp
    absfont/absfont_dump.cpp
    absfont/absfont_metrics.cpp
    absfont/absfont_path.cpp
    cfembed.cpp
    cffread.c
    cffread_abs.cpp
    cffwrite/cffwrite.cpp
    cffwrite/cffwrite_charset.cpp
    cffwrite/cffwrite_charset.h
    cffwrite/cffwrite_dict.cpp
    cffwrite/cffwrite_dict.h
    cffwrite/cffwrite_encoding.cpp
    cffwrite/cffwrite_encoding.h
    cffwrite/cffwrite_fdselect.cpp
    cffwrite/cffwrite_fdselect.h
    cffwrite/cffwrite_share.h
    cffwrite/cffwrite_sindex.cpp
    cffwrite/cffwrite_sindex.h
    cffwrite/cffwrite_subr.cpp
    cffwrite/cffwrite_subr.h
    cffwrite/cffwrite_t2cstr.cpp
    cffwrite/cffwrite_t2cstr.h
    cffwrite/cffwrite_varstore.cpp
    cffwrite/cffwrite_varstore.h
    ctutil.c
    da.c
    dynarr.c
    goadb.cpp
    support/except.c
    support/fixed.c
    nameread.cpp
    pdfwrite.cpp
    pstoken.cpp
    sfile.cpp
    sfntread.cpp
    sfntwrite.cpp
    sha1.c
    slogger.cpp
    smem.c
    svgwrite.cpp
    svread/sv_ops.h
    svread/sv_parse.y
    svread/svread.cpp
    t1cstr.cpp
    t1read.cpp
    t1read_keys.h
    t1write/t1write.cpp
    t1write/t1write_flexothers.h
    t1write/t1write_gcothers.h
    t1write/t1write_hintothers.h
    t1write/t1write_procsetothers.h
    t2cstr.cpp
    ttread.cpp
    tx_shared.cpp
    ufo_ops.h
    uforead.cpp
    ufowrite.cpp
    varread.cpp
    # include 
    include/abfdesc.h
    include/abferr.h
    include/absfont.h
    include/ceferr.h
    include/cfembed.h
    include/cffread_abs.h
    include/cffread.h
    include/cffwrite.h
    include/cfrerr.h
    include/cfwerr.h
    include/ctlshare.h
    include/ctutil.h
    include/da.h
    include/dynarr.h
    include/goadb.h
    include/nameread.h
    include/pdfwrite.h
    include/pdwerr.h
    include/psterr.h
    include/pstoken.h
    include/safetime.h
    include/sfile.h
    include/sfntread.h
    include/sfntwrite.h
    include/sfrerr.h
    include/sfwerr.h
    include/sha1.h
    include/slogger.h
    include/smem.h
    include/supportdefines.h
    include/supportexcept.h
    include/supportfp.h
    include/supportpublictypes.h
    include/svgwrite.h
    include/svread.h
    include/svrerr.h
    include/svwerr.h
    include/t1cerr.h
    include/t1cstr.h
    include/t1read.h
    include/t1rerr.h
    include/t1werr.h
    include/t1write.h
    include/t2cerr.h
    include/t2cstr.h
    include/ttread.h
    include/ttrerr.h
    include/tx_help
    include/tx_shared.h
    include/ufoerr.h
    include/uforead.h
    include/ufowerr.h
    include/ufowrite.h
    include/varread.h
)

set_property(TARGET shared PROPERTY CXX_STANDARD 17)

target_include_directories(shared PRIVATE include resource)

set_property(SOURCE absfont/absfont.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:ABF_DEBUG=1>)
set_property(SOURCE cfembed.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:CEF_DEBUG=1>)
set_property(SOURCE cffread_abs.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:CFR_DEBUG=1>)
set_property(SOURCE cffwrite/cffwrite.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:CFW_DEBUG=1>)
set_property(SOURCE dynarr.c PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:DNA_DEBUG=1>)
set_property(SOURCE pstoken.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:PST_DEBUG=1>)
set_property(SOURCE pdfwrite.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:PDW_DEBUG=1>)
set_property(SOURCE t1cstr.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:T1C_DEBUG=1>)
set_property(SOURCE t2cstr.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:T1C_DEBUG=1>)
set_property(SOURCE t1read.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:T1R_DEBUG=1>)
set_property(SOURCE t1write/t1write.c PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:T1W_DEBUG=1>)
set_property(SOURCE ttread.cpp PROPERTY
             COMPILE_DEFINITIONS $<$<CONFIG:Debug>:TTR_DEBUG=1>)

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(shared ${LIBXML2_TARGET})
endif()
