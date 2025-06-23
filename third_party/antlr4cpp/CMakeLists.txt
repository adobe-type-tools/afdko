include(FetchContent)

if(NOT DEFINED ANTLR4_TAG)
  # Set to branch name to keep library updated at the cost of needing to rebuild after 'clean'
  # Set to commit hash to keep the build stable and does not need to rebuild after 'clean'
  set(ANTLR4_TAG dev)
endif()

if(ANTLR4_ZIP_REPOSITORY)
  set(fetch_content_args
      URL ${ANTLR4_ZIP_REPOSITORY}
  )
else()
  set(fetch_content_args
      GIT_REPOSITORY https://github.com/antlr/antlr4.git
      GIT_TAG ${ANTLR4_TAG}
  )
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  list(APPEND fetch_content_args FIND_PACKAGE_ARGS CONFIG)
endif()
FetchContent_Declare(antlr4-runtime
    SOURCE_SUBDIR runtime/Cpp
    ${fetch_content_args}
)

option(ANTLR_BUILD_CPP_TESTS "Override: afdko" OFF)
option(ANTLR_BUILD_SHARED "Override: afdko" OFF)
option(ANTLR_BUILD_STATIC "Override: afdko" ON)

set(CMAKE_CXX_STANDARD 17)
FetchContent_MakeAvailable(antlr4-runtime)
add_library(afdko::antlr4 ALIAS antlr4_static)
