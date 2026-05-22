cmake_minimum_required(VERSION 3.21)
project(DailyHexPuzzleAndroid VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Quick QuickDialogs2)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)

# ── Android 15+: 16KB page alignment (required for API 35+ emulator/device) ─
if(ANDROID)
    add_link_options(-Wl,-z,max-page-size=16384)
endif()

# ── Solver C library ───────────────────────────────────────────────────────
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../solver-ffi
                 ${CMAKE_CURRENT_BINARY_DIR}/solver-ffi)

# ── Qt executable (shared library on Android, ELF on desktop) ─────────────
qt_add_executable(DailyHexPuzzleAndroid
    main.cpp
    HexBackend.h
    HexBackend.cpp
    HexSolverWorker.h
    HexSolverWorker.cpp
)

qt_add_qml_module(DailyHexPuzzleAndroid
    URI DailyHexPuzzle
    VERSION 1.0
    QML_FILES
        Main.qml
        HexBoardCanvas.qml
)

qt_add_resources(DailyHexPuzzleAndroid "fonts"
    PREFIX "/fonts"
    FILES
        fonts/NotoSansJP.ttf
)

target_include_directories(DailyHexPuzzleAndroid PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../solver-ffi
)

target_link_libraries(DailyHexPuzzleAndroid PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickDialogs2
    hex_solver
)

qt_import_plugins(DailyHexPuzzleAndroid
    INCLUDE_BY_TYPE
        platforms Qt6::QAndroidIntegrationPlugin
    EXCLUDE_BY_TYPE
        qmltooling
        networkinformation
        iconengines
        imageformats
        platforminputcontexts
        styles
)

set_target_properties(DailyHexPuzzleAndroid PROPERTIES
    QT_ANDROID_PACKAGE_NAME              "net.nanamitm.dailyhexpuzzle"
    QT_ANDROID_APP_NAME                  "Hex Puzzle"
    QT_ANDROID_VERSION_NAME              "${PROJECT_VERSION}"
    QT_ANDROID_VERSION_CODE              1
    QT_ANDROID_PACKAGE_SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/android"
    QT_ANDROID_MIN_SDK_VERSION           28
    QT_ANDROID_TARGET_SDK_VERSION        35
    QT_ANDROID_COMPILE_SDK_VERSION       36
    QT_ANDROID_SDK_BUILD_TOOLS_REVISION  36.0.0
)

# Qt-level APK signing for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND DEFINED ENV{QT_ANDROID_KEYSTORE_PATH})
    set_target_properties(DailyHexPuzzleAndroid PROPERTIES
        QT_ANDROID_SIGN_APK              TRUE
        QT_ANDROID_KEYSTORE_PATH         $ENV{QT_ANDROID_KEYSTORE_PATH}
        QT_ANDROID_KEYSTORE_ALIAS        $ENV{QT_ANDROID_KEYSTORE_ALIAS}
        QT_ANDROID_KEYSTORE_STORE_PASS   $ENV{QT_ANDROID_KEYSTORE_STORE_PASS}
        QT_ANDROID_KEYSTORE_KEY_PASS     $ENV{QT_ANDROID_KEYSTORE_KEY_PASS}
    )
endif()
