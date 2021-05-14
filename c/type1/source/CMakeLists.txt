add_executable(type1 type1.c)
install(TARGETS type1 DESTINATION bin)
if(WIN32)
    // Could be non-msvc compiler on windows
    target_compile_options(type1 PRIVATE -D_MSC_VER)
endif()
