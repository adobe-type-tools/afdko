add_subdirectory(hotconv)
add_subdirectory(typecomp)
add_subdirectory(pstoken)

add_library(makeotf OBJECT
    cb.cpp
    main.cpp
    fcdb.cpp
    cb.h
    fcdb.h
)

target_link_libraries(makeotf PRIVATE
                      $<TARGET_OBJECTS:hotconv>
                      $<TARGET_OBJECTS:pstoken>
                      $<TARGET_OBJECTS:typecomp>)

if (${NEED_LIBXML2_DEPEND})
    add_dependencies(makeotf ${LIBXML2_TARGET})
endif()

set_property(TARGET makeotf PROPERTY CXX_STANDARD 17)

target_include_directories(makeotf PUBLIC AFTER include ../shared/include ../shared/resource)

set_property(TARGET makeotf APPEND PROPERTY
             COMPILE_DEFINITIONS MAKEOTFLIB_EXPORTS=1)
