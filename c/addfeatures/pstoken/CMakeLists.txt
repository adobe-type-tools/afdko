add_library(pstoken OBJECT pstoken.c)

set_property(TARGET pstoken APPEND PROPERTY
             COMPILE_DEFINITIONS PSTOKEN_DEBUG=1 CFF_DEBUG=1 CFF_T13_SUPPORT=0)

target_include_directories(pstoken AFTER PRIVATE ../include ../../shared/include ../../shared/resource)
