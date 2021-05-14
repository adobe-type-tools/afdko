add_library(hotconv STATIC
    anon.c
    anon.h
    antlr.h
    BASE.c
    BASE.h
    CFF_.c
    CFF_.h
    cmap.c
    cmap.h
    codepage.h
    common.h
    config.h
    dlgauto.h
    dlgdef.h
    err.h
    featerr.c
    featgram.c
    feat.h
    featmode.h
    featscan.c
    feattoks.h
    GDEF.c
    GDEF.h
    GPOS.c
    GPOS.h
    GSUB.c
    GSUB.h
    head.c
    head.h
    hhea.c
    hhea.h
    hmtx.c
    hmtx.h
    hot.c
    map.h
    map.c
    maxp.c
    maxp.h
    name.c
    name.h
    OS_2.c
    OS_2.h
    otl.c
    otl.h
    post.c
    post.h
    schinese.h
    sfnt.c
    sfnt.h
    STAT.c
    STAT.h
    uniblock.h
    vhea.c
    vhea.h
    vmtx.c
    vmtx.h
    VORG.c
    VORG.h
    winansi.h
)

set_property(TARGET hotconv PROPERTY C_STANDARD 99)
target_compile_definitions(hotconv PRIVATE HOT_FEAT_SUPPORT=1)
target_include_directories(hotconv PRIVATE AFTER ../../utils/hotpccts/pccts/h)

