add_subdirectory(hotconv)
add_subdirectory(pstoken)

add_library(addfeatures OBJECT
    cb.cpp
    main.cpp
    fcdb.cpp
    cb.h
    fcdb.h
)

target_link_libraries(addfeatures PRIVATE
                      $<TARGET_OBJECTS:hotconv>
                      $<TARGET_OBJECTS:pstoken>)

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(addfeatures ${LIBXML2_TARGET})
endif()

set_property(TARGET addfeatures PROPERTY CXX_STANDARD 17)

target_include_directories(addfeatures PUBLIC AFTER include ../shared/include ../shared/resource)

set_property(TARGET addfeatures APPEND PROPERTY
             COMPILE_DEFINITIONS MAKEOTFLIB_EXPORTS=1)
