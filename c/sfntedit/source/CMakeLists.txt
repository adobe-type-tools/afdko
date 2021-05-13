add_executable(sfntedit main.c)

target_sources(sfntedit PRIVATE
    Eda.c
    Eda.h
    Efile.c
    Efile.h
    Eglobal.c
    Eglobal.h
    Emsgs.c
    Emsgs.h
    Esys.c
    Esys.h
    main.c
    otftableeditor.h
)

target_include_directories(sfntedit PRIVATE ../../spot/sfnt_includes ../../shared/include)
install(TARGETS sfntedit DESTINATION bin)
