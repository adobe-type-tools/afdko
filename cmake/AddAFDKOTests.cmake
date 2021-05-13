
find_package(Python3 COMPONENTS Interpreter)

if(Python3_Interpreter_FOUND)
    execute_process(
        COMMAND Python3::Interpreter -m pytest -h
        RESULT_VARIABLE HAS_PYTEST
    )
endif()

if(HAS_PYTEST AND BUILD_TESTING)
    if(WIN32)
        set(PERTESTENV "PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\;$ENV{PATH};PYTHONPATH=${CMAKE_SOURCE_DIR}/python\\;$ENV{PYTHONPATH};AFDKO_TEST_SKIP_CONSOLE=True")
    else()
        set(PERTESTENV "PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}:$ENV{PATH};PYTHONPATH=${CMAKE_SOURCE_DIR}/python:$ENV{PYTHONPATH};AFDKO_TEST_SKIP_CONSOLE=True")
    endif()
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    add_test(NAME runner_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/runner_test.py")
    add_test(NAME differ_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/differ_test.py")
    add_test(NAME buildcff2vf_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/buildcff2vf_test.py")
    add_test(NAME buildmasterotfs_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/buildmasterotfs_test.py")
    add_test(NAME checkoutlinesufo_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/checkoutlinesufo_test.py")
    add_test(NAME comparefamily_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/comparefamily_test.py")
    add_test(NAME convertfonttocid_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/convertfonttocid_test.py")
    add_test(NAME detype1_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/detype1_test.py")
    add_test(NAME fdkutils_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/fdkutils_test.py")
    add_test(NAME fontpdf_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/fontpdf_test.py")
    add_test(NAME makeinstancesufo_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/makeinstancesufo_test.py")
    add_test(NAME makeotfexe_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/makeotfexe_test.py")
    add_test(NAME makeotf_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/makeotf_test.py")
    add_test(NAME mergefonts_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/mergefonts_test.py")
    add_test(NAME otc2otf_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/otc2otf_test.py")
    add_test(NAME otf2otc_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/otf2otc_test.py")
    add_test(NAME otf2ttf_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/otf2ttf_test.py")
    add_test(NAME proofpdf_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/proofpdf_test.py")
    add_test(NAME rotatefont_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/rotatefont_test.py")
    add_test(NAME sfntdiff_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/sfntdiff_test.py")
    add_test(NAME sfntedit_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/sfntedit_test.py")
    add_test(NAME spot_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/spot_test.py")
    add_test(NAME ttfcomponentizer_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/ttfcomponentizer_test.py")
    add_test(NAME ttfdecomponentizer_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/ttfdecomponentizer_test.py")
    add_test(NAME ttxn_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/ttxn_test.py")
    add_test(NAME tx_test COMMAND Python3::Interpreter -m pytest "${CMAKE_SOURCE_DIR}/tests/tx_test.py")
    set_tests_properties(
        runner_test
        differ_test
        buildcff2vf_test
        buildmasterotfs_test
        checkoutlinesufo_test
        comparefamily_test
        convertfonttocid_test
        detype1_test
        fdkutils_test
        fontpdf_test
        makeinstancesufo_test
        makeotfexe_test
        makeotf_test
        mergefonts_test
        otc2otf_test
        otf2otc_test
        otf2ttf_test
        proofpdf_test
        rotatefont_test
        sfntdiff_test
        sfntedit_test
        spot_test
        ttfcomponentizer_test
        ttfdecomponentizer_test
        ttxn_test
        tx_test
        PROPERTIES ENVIRONMENT "${PERTESTENV}")
endif()
