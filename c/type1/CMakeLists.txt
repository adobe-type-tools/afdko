add_library(type1 OBJECT type1.c)

if(WIN32)
    # Could be non-msvc compiler on windows
    set_property(TARGET type1 APPEND PROPERTY COMPILE_DEFINITIONS _MSC_VER=1)
endif()
