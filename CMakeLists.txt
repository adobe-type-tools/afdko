cmake_minimum_required(VERSION 3.16)

project(afdko)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# This matches the MSVC_RUNTIME_LIBRARY property in
# c/makeotf/source/CMakeLists.txt, which can theoretically be used to
# statically link the runtime on Windows. However, the neededruntime DLL
# seems to be present in Windows and setting this caused problems in testing.
# (Setting ANTLR4_WITH_STATIC_CRT below to ON may or may not fix the problem.)
# cmake_policy(SET CMP0091 NEW) # Style of MSVC runtime selection

# RelWithDebInfo builds an optimized binary but includes debugging symbols
# Other common possibilities are "Debug" and "Release"
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type configuration" FORCE)
endif()
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)

# scikit-build
if(SKBUILD)
    find_package(PythonExtensions REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Antlr 4 configuration

# This is an alternate way of supplying the Antlr 4 sources that will override
# the git clone of the tag listed below. This is especially useful if you
# encounter compiler problems and need to make small edits to compensate. Start
# with the Antlr project's sources, e.g.
# https://www.antlr.org/download/antlr4-cpp-runtime-4.9.2-source.zip
# set(ANTLR4_ZIP_REPOSITORY "/path_to_antlr4_archive/a4.zip")

add_definitions(-DANTLR4CPP_STATIC)
set(ANTLR4_WITH_STATIC_CRT OFF)
# Use slightly more recent commit than 4.9.2 to deal with utfcpp test
# compilation problems
# set(ANTLR4_TAG tags/4.9.2)
set(ANTLR4_TAG 916f03366edf15bf8b50010b11d479c189bf9f96)
include(ExternalAntlr4Cpp)

# sanitizer support
# work around https://github.com/pypa/setuptools/issues/1928 with environment
# variable
if(DEFINED ENV{ADD_SANITIZER})
    set(ADD_SANITIZER "$ENV{ADD_SANITIZER}" CACHE STRING "Sanitizer type (\"none\" for no sanitizer")
else()
    set(ADD_SANITIZER "none" CACHE STRING "Sanitizer type (\"none\" for no sanitizer")
endif()
include(AddSanitizer)
add_sanitizer("${ADD_SANITIZER}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# Ported from old build files, XXX not sure if all of these are needed
if(WIN32)
    add_compile_definitions(CTL_CDECL=__cdecl CDECL=__cdecl
        OS=os_windowsNT ISP=isp_i80486 _CRT_SECURE_NO_DEPRECATE
        $<$<CONFIG:Release>:NDEBUG>)
endif()

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m floor "" HAVE_M_LIB)

include(CTest)
include(AddAFDKOTests)

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
