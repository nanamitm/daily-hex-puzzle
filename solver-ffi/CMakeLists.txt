cmake_minimum_required(VERSION 3.16)
project(hex_solver_ffi C)

# Emscripten links everything into the app; a shared library is not usable there
if(EMSCRIPTEN)
    add_library(hex_solver STATIC hex_solver.c)
    # The solver runs on a worker thread of the multithreaded Qt wasm build, and
    # its atomic cancel flag needs the 'atomics' feature to link with shared memory
    target_compile_options(hex_solver PRIVATE -pthread)
else()
    add_library(hex_solver SHARED hex_solver.c)
endif()
target_include_directories(hex_solver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(hex_solver PRIVATE _USE_MATH_DEFINES)

if(MSVC)
    target_compile_options(hex_solver PRIVATE /W3)
else()
    target_compile_options(hex_solver PRIVATE -Wall -O2)
    target_link_libraries(hex_solver PRIVATE m)
endif()

# Windows: also build an import library (.lib) alongside the DLL
set_target_properties(hex_solver PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

option(HEX_SOLVER_BUILD_TESTS "Build solver regression tests" OFF)
if(HEX_SOLVER_BUILD_TESTS)
    enable_testing()
    add_executable(hex_solver_tests tests/cancellation.c)
    target_link_libraries(hex_solver_tests PRIVATE hex_solver)
    add_test(NAME solver_cancellation COMMAND hex_solver_tests)
endif()
