cmake_minimum_required(VERSION 3.14)
project(afdko)
if(SKBUILD)
    find_package(PythonExtensions REQUIRED)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    add_compile_options(-DCTL_CDECL=__cdecl -DCDECL=__cdecl
        -DOS=os_windowsNT -DISP=isp_i80486 -D_CRT_SECURE_NO_DEPRECATE
        $<$<CONFIG:Release>:-DNDEBUG>)
endif()


add_subdirectory(c/shared/source)
add_subdirectory(c/detype1/source)
add_subdirectory(c/mergefonts/source)
add_subdirectory(c/rotatefont/source)
add_subdirectory(c/sfntdiff/source)
add_subdirectory(c/sfntedit/source)
add_subdirectory(c/spot/source)
add_subdirectory(c/tx/source)
add_subdirectory(c/type1/source)
add_subdirectory(c/makeotf)
