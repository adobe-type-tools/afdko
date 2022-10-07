add_library(type1 OBJECT type1.c)

target_include_directories(type1 PUBLIC AFTER ../shared/include)

if(WIN32)
    # Could be non-msvc compiler on windows
    set_property(TARGET type1 APPEND PROPERTY COMPILE_DEFINITIONS _MSC_VER=1)
endif()
