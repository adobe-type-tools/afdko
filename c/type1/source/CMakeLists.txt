add_executable(type1 type1.c)
install(TARGETS type1 DESTINATION bin)
if(WIN32)
    # Could be non-msvc compiler on windows
    target_compile_definitions(type1 PRIVATE _MSC_VER)
endif()
