set(GENERATED_SOURCES 
    FeatCtx.cpp
    FeatCtx.h
    FeatLexer.cpp
    FeatLexer.h
    FeatParserBaseVisitor.cpp
    FeatParserBaseVisitor.h
    FeatParser.cpp
    FeatParser.h
    FeatParserVisitor.cpp
    FeatParserVisitor.h
)

add_library(hotconv STATIC
    anon.c
    anon.h
    BASE.c
    BASE.h
    CFF_.c
    CFF_.h
    cmap.c
    cmap.h
    common.h
    feat.h
    FeatCtx.cpp
    FeatCtx.h
    ${GENERATED_SOURCES}
    FeatVisitor.cpp
    FeatVisitor.h
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
    hotmap.h
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
target_include_directories(hotconv PRIVATE AFTER $<$<COMPILE_LANGUAGE:CXX>:${ANTLR4_INCLUDE_DIRS}>)
target_link_libraries(hotconv PUBLIC antlr4_static)

if ((${LibXml2_FOUND}) AND (NOT "${CMAKE_SYSTEM}" MATCHES "Linux"))
    target_link_libraries(hotconv PUBLIC ${LIBXML2_LIBRARY})
else ()
    add_dependencies(hotconv ${LIBXML2_TARGET})
    if (WIN32)
       target_link_libraries(hotconv PUBLIC ${LIBXML2_WIN_LIBRARIES})
    else ()
       target_link_libraries(hotconv PUBLIC ${LIBXML2_NONWIN_LIBRARIES})
    endif()
endif()

if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(hotconv PRIVATE -Wall -Wno-attributes)
    # set_source_files_properties(${GENERATED_SOURCES} PROPERTIES COMPILE_FLAGS -Wno-attributes)
endif()
