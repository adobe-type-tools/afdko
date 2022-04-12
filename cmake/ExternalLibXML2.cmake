# set(libxml2_release "2.9")
# set(libxml2_patch_version 0)
# set(libxml2_url "ftp://xmlsoft.org/libxml2/libxml2-sources-${libxml2_release}.${libxml2_patch_version}.tar.gz")
# set(libxml2_md5 "7da7af8f62e111497d5a2b61d01bd811")

# #We need to state that we're dependent on ZLib so build order is correct
# set(_XML2_DEPENDS ZLib)

#This build requires make, ensure we have it, or error out.
# if(CMAKE_GENERATOR MATCHES ".*Makefiles")
#   set(MAKE_EXECUTABLE "$(MAKE)")
# else()
#   find_program(MAKE_EXECUTABLE make)
#   if(NOT MAKE_EXECUTABLE)
#     message(FATAL_ERROR "Could not find 'make', required to build libxml2.")
#   endif()
# endif()

# include (ExternalProject)
# ExternalProject_Add(libxml2
#   DEPENDS ${_XML2_DEPENDS}
#   URL ${libxml2_url}
#   URL_MD5 ${libxml2_md5}
#   PREFIX  ${test_ext_proj_BUILD_PREFIX}
#   DOWNLOAD_DIR ${test_ext_proj_DOWNLOAD_DIR}
#   INSTALL_DIR  ${test_ext_proj_BUILD_INSTALL_PREFIX}
#   BUILD_IN_SOURCE 1
#   CONFIGURE_COMMAND ./configure
#     --prefix=${test_ext_proj_BUILD_INSTALL_PREFIX}
#     --with-zlib=${ZLIB_ROOT}
#   BUILD_COMMAND ${MAKE_EXECUTABLE}
#   INSTALL_COMMAND ${MAKE_EXECUTABLE} install
# )

set(LIBXML2_TARGET external.libxml2)
set(LIBXML2_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIBXML2_TARGET})
set(LIBXML2_SRC_DIR ${LIBXML2_INSTALL_DIR}/src/${LIBXML2_TARGET})
set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INSTALL_DIR}/include/libxml2)
set(LIBXML2_LIBRARY
        ${LIBXML2_INSTALL_DIR}/lib/libxml2.a)
include_directories(${LIBXML2_INCLUDE_DIRS})
# set(LIBXML2_BINARY_DIR ${LIBXML2_INSTALL_DIR}/kamile_build)

message("LIBXML2_INSTALL_DIR: ${LIBXML2_INSTALL_DIR}")
message("LIBXML2_SRC_DIR: ${LIBXML2_SRC_DIR}")
message("LIBXML2_INCLUDE_DIRS: ${LIBXML2_INCLUDE_DIRS}")

set(LIBXML2_GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git)
if(NOT DEFINED LIBXML2_TAG)
  set(LIBXML2_TAG master)
endif()

# list(APPEND LIBXML2_LIBRARIES xml2)
# foreach(lib IN LISTS LIBXML2_LIBRARIES)
#   list(APPEND LIBXML2_BUILD_BYPRODUCTS ${LIBXML2_INSTALL_DIR}/lib/lib${lib}.a)

#   add_library(${lib} STATIC IMPORTED)
#   set_property(TARGET ${lib} PROPERTY IMPORTED_LOCATION
#                ${LIBXML2_INSTALL_DIR}/lib/lib${lib}.a)
#   add_dependencies(${lib} ${LIBXML2_TARGET})
#   message("Byproduct library: ${lib}")
#   message(${lib})
# endforeach(lib)
# message("LIBXML2_BUILD_BYPRODUCTS: ${LIBXML2_BUILD_BYPRODUCTS}")

message("\nSOURCE DIR: ${LIBXML2_SRC_DIR}/autogen.sh\n")
include (ExternalProject)
ExternalProject_Add(${LIBXML2_TARGET}
  PREFIX ${LIBXML2_TARGET}
  GIT_REPOSITORY ${LIBXML2_GIT_REPOSITORY}
  GIT_TAG ${LIBXML2_TAG}
  SOURCE_DIR ${LIBXML2_SRC_DIR}
  CONFIGURE_COMMAND ${LIBXML2_SRC_DIR}/autogen.sh --without-python
                                                  --prefix=${LIBXML2_INSTALL_DIR}
  BYPRODUCTS ${LIBXML2_LIBRARY}
  BUILD_COMMAND make
  INSTALL_COMMAND make install
  # WORKING_DIRECTORY ${LIBXML2_SRC_DIR}
  # BUILD_IN_SOURCE 1
)

ExternalProject_Get_Property(${LIBXML2_TARGET} SOURCE_DIR)
# ExternalProject_Get_Property(sphinxbase BINARY_DIR)
# SET(SPHINXBASE_SOURCE_DIR ${SOURCE_DIR})
# SET(SPHINXBASE_BINARY_DIR ${BINARY_DIR})
# SET(SPHINXBASE_LIBRARIES ${SPHINXBASE_BINARY_DIR}/src/libsphinxbase/.libs/libsphinxbase.a)
# INCLUDE_DIRECTORIES(${SPHINXBASE_SOURCE_DIR}/include)

add_library(libxml2 STATIC IMPORTED)
set_property(TARGET libxml2 PROPERTY IMPORTED_LOCATION ${LIBXML2_LIBRARY}
            )
add_dependencies(libxml2 ${LIBXML2_TARGET})

# include_directories(${LIBXML2_INSTALL_DIR}/include)

# add_dependencies(mergefonts ${LIBXML2_LIBRARY})

# ExternalProject_Add_Step(${LIBXML2_TARGET}
#   PREFIX ${LIBXML2_TARGET}
#   CONFIGURE_COMMAND ${LIBXML2_SRC_DIR}/autogen.sh --without-python --prefix=${LIBXML2_INSTALL_DIR}
#   BUILD_COMMAND make
#   WORKING_DIRECTORY ${LIBXML2_SRC_DIR}
# )

# ExternalProject_Add_Step(
#   ${LIBXML2_TARGET}
#   CONFIGURE_COMMAND <SOURCE_DIR>/autogen.sh --without-python
# )

# ExternalProject_Get_Property(${LIBXML2_TARGET} BINARY_DIR)
# file(MAKE_DIRECTORY ${LIBXML2_SRC_DIR})
# set(libxml2_static ${LIBXML2_SRC_DIR}/libOpenCL.so
#   CACHE PATH "libxml2 statuc library" FORCE
# )

# add_library(libxml_static SHARED IMPORTED)
# add_dependencies(libxml_static ${LIBXML2_TARGET})
# set_target_properties(libxml_static
#     PROPERTIES IMPORTED_LOCATION ${LIBXML2_BUILD_BYPRODUCTS}
# )

# target_link_libraries()
# add_library(${LIBXML2_TARGET})