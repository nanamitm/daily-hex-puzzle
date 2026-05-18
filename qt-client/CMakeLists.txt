cmake_minimum_required(VERSION 3.16)
project(DailyHexPuzzle VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ── solver-ffi shared library ──────────────────────────────────────────────
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../solver-ffi
                 ${CMAKE_CURRENT_BINARY_DIR}/solver-ffi)

# ── Windows exe icon: generate app.ico via PowerShell, embed via app.rc ───
if(WIN32)
    set(APP_ICO ${CMAKE_CURRENT_SOURCE_DIR}/app.ico)
    add_custom_command(
        OUTPUT  ${APP_ICO}
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File ${CMAKE_CURRENT_SOURCE_DIR}/gen_icon.ps1
                -OutDir ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating app.ico"
        VERBATIM
    )
    add_custom_target(gen_app_icon DEPENDS ${APP_ICO})
endif()

# ── Qt client executable ───────────────────────────────────────────────────
qt_add_executable(DailyHexPuzzle WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.h
    mainwindow.cpp
    hexboardwidget.h
    hexboardwidget.cpp
    solverworker.h
    solverworker.cpp
    $<$<BOOL:${WIN32}>:app.rc>
)

if(WIN32)
    add_dependencies(DailyHexPuzzle gen_app_icon)
endif()

target_link_libraries(DailyHexPuzzle PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    hex_solver
)

# MSVC does not define M_PI by default
if(MSVC)
    target_compile_definitions(DailyHexPuzzle PRIVATE _USE_MATH_DEFINES)
endif()

# ── post-build: copy DLL next to the executable (Windows) ─────────────────
if(WIN32)
    add_custom_command(TARGET DailyHexPuzzle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:hex_solver>
            $<TARGET_FILE_DIR:DailyHexPuzzle>
        COMMENT "Copying hex_solver.dll")
endif()

# ── macOS RPATH ────────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(DailyHexPuzzle PROPERTIES
        INSTALL_RPATH "@executable_path")
endif()

# ── Linux RPATH ────────────────────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    set_target_properties(DailyHexPuzzle PROPERTIES
        INSTALL_RPATH "$ORIGIN")
endif()
