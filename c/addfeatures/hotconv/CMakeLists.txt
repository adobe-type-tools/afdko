add_library(hotconv OBJECT
    # generated
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
    # static
    anon.cpp
    anon.h
    BASE.cpp
    BASE.h
    cmap.cpp
    cmap.h
    common.h
    FeatCtx.cpp
    FeatCtx.h
    ${GENERATED_SOURCES}
    FeatVisitor.cpp
    FeatVisitor.h
    GDEF.cpp
    GDEF.h
    GPOS.cpp
    GPOS.h
    GSUB.cpp
    GSUB.h
    head.cpp
    head.h
    hmtx.cpp
    hmtx.h
    hot.cpp
    hotlogger.h
    hotlogger.cpp
    hotmap.h
    map.cpp
    maxp.cpp
    maxp.h
    name.cpp
    name.h
    MVAR.h
    MVAR.cpp
    OS_2.cpp
    OS_2.h
    otl.cpp
    otl.h
    post.cpp
    post.h
    schinese.h
    sfnt.cpp
    sfnt.h
    STAT.cpp
    STAT.h
    uniblock.h
    vmtx.cpp
    vmtx.h
    winansi.h
)

set_property(TARGET hotconv PROPERTY C_STANDARD 99)
set_property(TARGET hotconv PROPERTY CXX_STANDARD 17)

target_include_directories(hotconv BEFORE PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${ANTLR4_INCLUDE_DIRS}>)
target_include_directories(hotconv AFTER PRIVATE ../include ../../shared/include ../../shared/resource)
# Because hotconv is an object library, all this should do is
# establish the build dependency
target_link_libraries(hotconv PUBLIC antlr4_static)

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(hotconv ${LIBXML2_TARGET})
endif()

if ( CMAKE_COMPILER_IS_GNUCC )
        set_property(TARGET hotconv APPEND PROPERTY
                     COMPILE_OPTIONS -Wall -Wno-attributes)
endif()
