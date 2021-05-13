
function(add_sanitizer san)
    if("${san}" STREQUAL "none")
        return()
    endif()

    include(CheckCCompilerFlag)
    include(CMakePushCheckState)

    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=${san}")
    check_c_compiler_flag("-fsanitize=${san} -fno-omit-frame-pointer" "_add_sanitizer_${san}")
    cmake_pop_check_state(RESET)

    if (NOT _add_sanitizer_${san})
        message(FATAL_ERROR "Could not find sanitizer ${san}")
    endif()

    add_compile_options(-fsanitize=${san} -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${san} -fno-omit-frame-pointer)
endfunction()
