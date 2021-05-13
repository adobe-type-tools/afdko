add_executable(sfntdiff Dmain.c)

target_sources(sfntdiff PRIVATE
    Dda.c
    Dda.h
    Ddesc.c
    Ddesc.h
    Dfile.c
    Dfile.h
    Dglobal.c
    Dglobal.h
    Dhead.c
    Dhead.h
    Dname.c
    Dname.h
    Dopt.c
    Dopt.h
    Dpriv.c
    Dsfnt.c
    Dsfnt.h
    Dsys.c
    Dsys.h
    Dtto.c
    Dtto.h
    numtypes.h
)

target_include_directories(sfntdiff PRIVATE ../../spot/sfnt_includes ../../shared/include)
install(TARGETS sfntdiff DESTINATION bin)
