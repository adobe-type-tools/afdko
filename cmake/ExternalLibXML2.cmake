set(LIBXML2_TARGET external.libxml2)
set(LIBXML2_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIBXML2_TARGET})
set(LIBXML2_SRC_DIR ${LIBXML2_INSTALL_DIR}/src/${LIBXML2_TARGET})
set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INSTALL_DIR}/include/libxml2)
set(LIBXML2_LIBRARY
        ${LIBXML2_INSTALL_DIR}/lib/libxml2.a)
include_directories(${LIBXML2_INCLUDE_DIRS})

set(LIBXML2_GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git)
if(NOT DEFINED LIBXML2_TAG)
  set(LIBXML2_TAG master)
endif()

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
)

ExternalProject_Get_Property(${LIBXML2_TARGET} SOURCE_DIR)

add_library(libxml2 STATIC IMPORTED)
set_property(TARGET libxml2 PROPERTY IMPORTED_LOCATION ${LIBXML2_LIBRARY}
            )
add_dependencies(libxml2 ${LIBXML2_TARGET})
