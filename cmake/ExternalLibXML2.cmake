include(ExternalProject)
cmake_policy(SET CMP0114 NEW)  # Added by iterumllc

# Edits below are for path length or (in the zip download) correctness

set(LIBXML2_ROOT ${CMAKE_CURRENT_BINARY_DIR}/l2/src/l2)  # Edited by iterumllc
set(LIBXML2_INCLUDE_DIRS ${LIBXML2_ROOT}/include/libxml2)
include_directories(${LIBXML2_INCLUDE_DIRS})
set(LIBXML2_GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git)
if(NOT DEFINED LIBXML2_TAG)
  # Set to branch name to keep library updated at the cost of needing to rebuild after 'clean'
  # Set to commit hash to keep the build stable and does not need to rebuild after 'clean'
  set(LIBXML2_TAG master)
endif()

if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
  set(LIBXML2_OUTPUT_DIR ${LIBXML2_ROOT}/$(Configuration))
elseif(${CMAKE_GENERATOR} MATCHES "Xcode.*")
  set(LIBXML2_OUTPUT_DIR ${LIBXML2_ROOT}/$(CONFIGURATION))
else()
  set(LIBXML2_OUTPUT_DIR ${LIBXML2_ROOT})
endif()


if(MSVC)
  set(LIBXML2_STATIC_LIBRARIES
      ${LIBXML2_OUTPUT_DIR}/LIBXML2-runtime-static.lib)
  set(LIBXML2_SHARED_LIBRARIES
      ${LIBXML2_OUTPUT_DIR}/LIBXML2-runtime.lib)
  set(LIBXML2_RUNTIME_LIBRARIES
      ${LIBXML2_OUTPUT_DIR}/LIBXML2-runtime.dll)
else()
  set(LIBXML2_STATIC_LIBRARIES
      ${LIBXML2_OUTPUT_DIR}/libLIBXML2-runtime.a)
  if(MINGW)
    set(LIBXML2_SHARED_LIBRARIES
        ${LIBXML2_OUTPUT_DIR}/libLIBXML2-runtime.dll.a)
    set(LIBXML2_RUNTIME_LIBRARIES
        ${LIBXML2_OUTPUT_DIR}/libLIBXML2-runtime.dll)
  elseif(CYGWIN)
    set(LIBXML2_SHARED_LIBRARIES
        ${LIBXML2_OUTPUT_DIR}/libLIBXML2-runtime.dll.a)
    set(LIBXML2_RUNTIME_LIBRARIES
        ${LIBXML2_OUTPUT_DIR}/cygLIBXML2-runtime-4.9.2.dll)
  elseif(APPLE)
    set(LIBXML2_RUNTIME_LIBRARIES
        ${LIBXML2_OUTPUT_DIR}/libLIBXML2-runtime.dylib)
  else()
    set(LIBXML2_RUNTIME_LIBRARIES
        ${LIBXML2_OUTPUT_DIR}/libLIBXML2-runtime.so)
  endif()
endif()


if(${CMAKE_GENERATOR} MATCHES ".* Makefiles")
  # This avoids
  # 'warning: jobserver unavailable: using -j1. Add '+' to parent make rule.'
  set(LIBXML2_BUILD_COMMAND $(MAKE))
elseif(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
  set(LIBXML2_BUILD_COMMAND
      ${CMAKE_COMMAND}
          --build .
          --config $(Configuration)
          --target)
elseif(${CMAKE_GENERATOR} MATCHES "Xcode.*")
  set(LIBXML2_BUILD_COMMAND
      ${CMAKE_COMMAND}
          --build .
          --config $(CONFIGURATION)
          --target)
else()
  set(LIBXML2_BUILD_COMMAND
      ${CMAKE_COMMAND}
          --build .
          --target)
endif()


# if(NOT DEFINED LIBXML2_WITH_STATIC_CRT)
#   set(LIBXML2_WITH_STATIC_CRT ON)
# endif()


ExternalProject_Add(
    libxml2_runtime
    PREFIX l2  # Edited by iterumllc
    GIT_REPOSITORY ${LIBXML2_GIT_REPOSITORY}
    GIT_TAG ${LIBXML2_TAG}
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
    BUILD_COMMAND ""
    BUILD_IN_SOURCE 1
    SOURCE_DIR ${LIBXML2_ROOT}
    SOURCE_SUBDIR macos/src
    # CMAKE_CACHE_ARGS
    #     -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    #     -DWITH_STATIC_CRT:BOOL=${LIBXML2_WITH_STATIC_CRT}
        # -DCMAKE_CXX_STANDARD:STRING=17 # if desired, compile the runtime with a different C++ standard
        # -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD} # alternatively, compile the runtime with the same C++ standard as the outer project
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL 1)


# Separate build step as rarely people want both
set(LIBXML2_BUILD_DIR ${LIBXML2_ROOT})
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
  # CMake 3.14 builds in above's SOURCE_SUBDIR when BUILD_IN_SOURCE is true
  set(LIBXML2_BUILD_DIR ${LIBXML2_ROOT})
endif()

message("----LINE 135---\n")

ExternalProject_Add_Step(
    libxml2_runtime
    build_static
    COMMAND ${LIBXML2_BUILD_COMMAND} libxml2_static
    # Depend on target instead of step (a custom command)
    # to avoid running dependent steps concurrently
    DEPENDS libxml2_runtime
    BYPRODUCTS ${LIBXML2_STATIC_LIBRARIES}
    EXCLUDE_FROM_MAIN 1
    WORKING_DIRECTORY ${LIBXML2_BUILD_DIR})
ExternalProject_Add_StepTargets(libxml2_runtime)

message("----LINE 149---\n")

add_library(libxml2_static STATIC IMPORTED)
add_dependencies(libxml2_static libxml2_runtime-build_static)
set_target_properties(libxml2_static PROPERTIES
                      IMPORTED_LOCATION ${LIBXML2_STATIC_LIBRARIES})

ExternalProject_Add_Step(
    libxml2_runtime
    build_shared
    COMMAND ${LIBXML2_BUILD_COMMAND} libxml2_shared
    # Depend on target instead of step (a custom command)
    # to avoid running dependent steps concurrently
    DEPENDS libxml2_runtime
    BYPRODUCTS ${LIBXML2_SHARED_LIBRARIES} ${LIBXML2_RUNTIME_LIBRARIES}
    EXCLUDE_FROM_MAIN 1
    WORKING_DIRECTORY ${LIBXML2_BUILD_DIR})
ExternalProject_Add_StepTargets(libxml2_runtime)

add_library(libxml2_shared SHARED IMPORTED)
add_dependencies(libxml2_shared libxml2_runtime-build_shared)
set_target_properties(libxml2_shared PROPERTIES
                      IMPORTED_LOCATION "${LIBXML2_RUNTIME_LIBRARIES}")
if(LIBXML2_SHARED_LIBRARIES)
  set_target_properties(libxml2_shared PROPERTIES
                        IMPORTED_IMPLIB "${LIBXML2_SHARED_LIBRARIES}")
endif()

message("----SUCCESS!!---\n")