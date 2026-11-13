cmake_minimum_required(VERSION 3.20)

project(MS2Beam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MS2_BUILD_TESTS "Build the MS2 test suite" ON)
option(MS2_BUILD_CLI "Build the MS2 command-line interface" ON)

add_library(ms2_core
    src/cpp/src/Matrix.cpp
    src/cpp/src/Element.cpp
    src/cpp/src/Model.cpp
    src/cpp/src/Assembler.cpp
    src/cpp/src/BoundaryCondition.cpp
    src/cpp/src/LinearSolver.cpp
    src/cpp/src/PostProcessor.cpp
    src/cpp/src/BeamSolver.cpp
    src/cpp/src/InputParser.cpp
    src/cpp/src/OutputWriter.cpp
)

target_include_directories(ms2_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/include
)

target_include_directories(ms2_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ms2_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MS2_BUILD_CLI)
    add_executable(ms2_cli src/cpp/main.cpp)
    target_link_libraries(ms2_cli PRIVATE ms2_core)
endif()

if(MS2_BUILD_TESTS)
    enable_testing()

    add_executable(ms2_tests
        tests/cpp/test_main.cpp
        tests/cpp/test_matrix.cpp
        tests/cpp/test_element.cpp
        tests/cpp/test_cantilever.cpp
        tests/cpp/test_examples.cpp
        tests/cpp/test_io.cpp
    )

    target_include_directories(ms2_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/catch2
    )

    target_link_libraries(ms2_tests PRIVATE ms2_core)

    add_test(NAME ms2_tests COMMAND ms2_tests)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND AND MS2_BUILD_CLI)
        add_test(
            NAME ms2_backend
            COMMAND ${Python3_EXECUTABLE} -m unittest discover
                    -s ${CMAKE_CURRENT_SOURCE_DIR}/tests/python -v
        )
        set_tests_properties(ms2_backend PROPERTIES
            ENVIRONMENT "MS2_CLI=$<TARGET_FILE:ms2_cli>"
        )
    endif()
endif()
