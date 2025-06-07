include(FetchContent)

if(NOT DEFINED LIBXML2_TAG)
  set(LIBXML2_TAG master)
endif()

set(fetch_content_args)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24 AND NOT DEFINED ENV{FORCE_BUILD_LIBXML2})
  list(APPEND fetch_content_args FIND_PACKAGE_ARGS CONFIG)
endif()
FetchContent_Declare(LibXml2
        GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git
        GIT_TAG ${LIBXML2_TAG}
)

option(AFDKO_BUILD_LIBXML2_SHARED_LIB
        "Whether to use the shared library of afdko" OFF)
if(AFDKO_BUILD_LIBXML2_SHARED_LIB)
  set(BUILD_SHARED_LIBS ON)
else()
  set(BUILD_SHARED_LIBS OFF)
endif()
option(LIBXML2_WITH_DEBUG "Override: afdko" OFF)
option(LIBXML2_WITH_DOCS "Override: afdko" OFF)
option(LIBXML2_WITH_TESTS "Override: afdko" OFF)
option(LIBXML2_WITH_PYTHON "Override: afdko" OFF)
option(LIBXML2_WITH_ICONV "Override: afdko" OFF)
option(LIBXML2_WITH_LZMA "Override: afdko" OFF)
option(LIBXML2_WITH_ZLIB "Override: afdko" OFF)

if(CMAKE_VERSION VERSION_LESS 3.24 AND DEFINED ENV{FORCE_SYSTEM_LIBXML2})
  find_package(LibXml2 REQUIRED CONFIG)
else()
  if(DEFINED ENV{FORCE_SYSTEM_LIBXML2})
    set(CMAKE_REQUIRE_FIND_PACKAGE_LibXml2 ON)
  endif()
  FetchContent_MakeAvailable(LibXml2)
endif()
