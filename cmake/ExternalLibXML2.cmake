include (ExternalProject)
cmake_policy(SET CMP0114 NEW)

set(LIBXML2_TARGET external.libxml2)

set(LIBXML2_GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git)
if(NOT DEFINED LIBXML2_TAG)
  set(LIBXML2_TAG master)
endif()

if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
  set(LIBXML2_OUTPUT_DIR ${LIBXML2_TARGET}/bin/${CMAKE_BUILD_TYPE})
else()
  set(LIBXML2_OUTPUT_DIR ${LIBXML2_TARGET}/bin)
endif()

if(WIN32)
  set(LIBXML2_LIBNAME libxml2s.lib)
else()
  set(LIBXML2_LIBNAME libxml2.a)
endif()

ExternalProject_Add(${LIBXML2_TARGET}
  GIT_REPOSITORY ${LIBXML2_GIT_REPOSITORY}
  GIT_TAG ${LIBXML2_TAG}
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${LIBXML2_OUTPUT_DIR}/${LIBXML2_LIBNAME}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIBXML2_TARGET}/src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIBXML2_TARGET}/bin
  CMAKE_ARGS -D BUILD_SHARED_LIBS=OFF -D CMAKE_BUILD_TYPE=Release -D LIBXML2_WITH_ICONV=OFF -D LIBXML2_WITH_LZMA=OFF -D LIBXML2_WITH_ZLIB=OFF -D LIBXML2_WITH_PYTHON=OFF -D CMAKE_VERBOSE_MAKEFILE:BOOL=ON
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(${LIBXML2_TARGET} BINARY_DIR)
ExternalProject_Get_Property(${LIBXML2_TARGET} SOURCE_DIR)

file(MAKE_DIRECTORY ${SOURCE_DIR}/include)

add_library(libxml2 STATIC IMPORTED GLOBAL)

add_dependencies(libxml2 ${LIBXML2_TARGET})
set_target_properties(libxml2 PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${LIBXML2_OUTPUT_DIR}/${LIBXML2_LIBNAME}
  INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/include)
target_include_directories(libxml2 INTERFACE ${SOURCE_DIR}/include INTERFACE ${BINARY_DIR})

set(LIBXML2_STATIC_INCLUDE_DIR ${SOURCE_DIR}/include ${BINARY_DIR})

set(LIBXML2_WIN_LIBRARIES wsock32 ws2_32 libxml2)

set(LIBXML2_NONWIN_LIBRARIES libxml2)
