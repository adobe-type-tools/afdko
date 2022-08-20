add_cython_target(cython_code _internal.pyx)

add_library(adapter OBJECT ${cython_code})

target_include_directories(adapter PRIVATE ${PYTHON_INCLUDE_DIRS})
