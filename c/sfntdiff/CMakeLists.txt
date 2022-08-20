add_library(sfntdiff OBJECT
    Dglobal.c
    Dglobal.h
    Dfile.h
    Dfile.c
    Dhead.h
    Dhead.c
    Dmain.c
    Dname.c
    Dname.h
    Dpriv.c
    Dsfnt.c
    Dsfnt.h
)

target_include_directories(sfntdiff AFTER PRIVATE ../spot ../spot/sfnt_includes ../shared/include)
