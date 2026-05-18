cmake_minimum_required(VERSION 3.16)
project(hex_solver_ffi C)

add_library(hex_solver SHARED hex_solver.c)
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
