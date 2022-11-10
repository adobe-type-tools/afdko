cmake_minimum_required(VERSION 3.16)

project(afdko)

# Change and uncomment to build directly with cmake (outside of pip)
# set(CMAKE_MODULE_PATH /usr/lib/python3.10/site-packages/skbuild/resources/cmake)
# set(CMAKE_MODULE_PATH /opt/homebrew/lib/python3.9/site-packages/skbuild/resources/cmake)
# set(CMAKE_FIND_FRAMEWORK_EXTRA_LOCATIONS /opt/homebrew/Cellar/python@3.9/3.9.13_4/Frameworks/Python.framework)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# RelWithDebInfo builds an optimized binary but includes debugging symbols
# Other common possibilities are "Debug" and "Release"
if (NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type configuration" FORCE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type configuration" FORCE)
endif()
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)

# scikit-build
find_package(PythonLibs REQUIRED)
find_package(PythonExtensions REQUIRED)
find_package(Cython REQUIRED)
set(SKBUILD_LINK_LIBRARIES_KEYWORD PRIVATE)

# add_compile_definitions(HOT_DEBUG)

add_compile_definitions(STRIP_EXTERN_C)

# Uncomment to export main__XXX functions in shared object on Linux
# function(_set_python_extension_symbol_visibility _target)
#   message("Skipping _set_python_extension_symbol_visibility")
# endfunction()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Antlr 4 configuration

# This is an alternate way of supplying the Antlr 4 sources that will override
# the git clone of the tag listed below. This is especially useful if you
# encounter compiler problems and need to make small edits to compensate. Start
# with the Antlr project's sources, e.g.
# https://www.antlr.org/download/antlr4-cpp-runtime-4.9.3-source.zip
# set(ANTLR4_ZIP_REPOSITORY "/path_to_antlr4_archive/a4.zip")

add_compile_definitions(ANTLR4CPP_STATIC)
set(ANTLR4_WITH_STATIC_CRT OFF)
# set(ANTLR4_TAG tags/4.13.2)
# This is a more recent commit than 4.13.2 that addresses a missing
# include directive needed in more recent Visual Studio releases
set(ANTLR4_TAG df4d68c09cdef73e023b8838a8bc7ca4dff1d1de)
include(ExternalAntlr4Cpp)

# libxml2 configuration

if (DEFINED ENV{FORCE_BUILD_LIBXML2})
    set(BUILD_STATIC_LIBXML2 TRUE)
else()
    FIND_PACKAGE(LibXml2)
    if (${LibXml2_FOUND})
        set(BUILD_STATIC_LIBXML2 FALSE)
    elseif (DEFINED ENV{FORCE_SYSTEM_LIBXML2})
        message(FATAL_ERROR "FORCE_SYSTEM_LIBXML2 set but LibXML2 not found")
    else()
        set(BUILD_STATIC_LIBXML2 TRUE)
    endif()
endif()

if (${BUILD_STATIC_LIBXML2})
    message(STATUS "LibXML2 not found or overridden -- will download and statically link")
    set(LIBXML2_TAG tags/v2.9.13)
    include(ExternalLibXML2)
    include_directories(${LIBXML2_STATIC_INCLUDE_DIR})
    set(NEED_LIBXML2_DEPEND TRUE)
    if (WIN32)
        add_compile_definitions(LIBXML_STATIC)
        set(CHOSEN_LIBXML2_LIBRARY ${LIBXML2_WIN_LIBRARIES})
    else()
        set(CHOSEN_LIBXML2_LIBRARY ${LIBXML2_NONWIN_LIBRARIES})
    endif()
else()
    include_directories(${LIBXML2_INCLUDE_DIR})
    set(NEED_LIBXML2_DEPEND FALSE)
    set(CHOSEN_LIBXML2_LIBRARY ${LIBXML2_LIBRARY})
endif()

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

# Ported from old build files, XXX not sure if all of these are needed
if(WIN32)
    add_compile_definitions(CTL_CDECL=__cdecl OS=os_windowsNT $<$<CONFIG:Release>:NDEBUG>)
endif()

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m floor "" HAVE_M_LIB)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(_internal MODULE)
python_extension_module(_internal)

if (HAVE_M_LIB)
    target_link_libraries(_internal PRIVATE m)
endif ()

target_link_libraries(_internal PRIVATE ${CHOSEN_LIBXML2_LIBRARY})

add_subdirectory(cython)
add_subdirectory(c/shared)
add_subdirectory(c/detype1)
add_subdirectory(c/mergefonts)
add_subdirectory(c/rotatefont)
add_subdirectory(c/sfntdiff)
add_subdirectory(c/sfntedit)
add_subdirectory(c/spot)
add_subdirectory(c/type1)
add_subdirectory(c/addfeatures)

target_link_libraries(_internal PRIVATE adapter detype1 type1 addfeatures spot sfntedit sfntdiff mergefonts rotatefont shared antlr4_static)

install(TARGETS _internal LIBRARY DESTINATION python/afdko)
