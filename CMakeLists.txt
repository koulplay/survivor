cmake_minimum_required(VERSION 3.25)
project(survivor LANGUAGES C CXX)   # C нужен, чтобы собрать саму raylib

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- зависимости: скачиваются и собираются при первой загрузке проекта ---
# Нужен интернет при первой конфигурации; Git не нужен (качаются архивы).
include(FetchContent)

# raylib 6.0
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # примеры raylib не собираем
FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM                                    # не показывать предупреждения из заголовков raylib
)
FetchContent_MakeAvailable(raylib)

# Catch2 3.16.1
FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Catch2)

# --- предупреждения ---
if (MSVC)
    set(SURVIVOR_WARNINGS /W4 /utf-8)
else()
    set(SURVIVOR_WARNINGS -Wall -Wextra -Wpedantic)
endif()

# --- логика игры: библиотека, её подключают игра и тесты ---
add_library(survivor_core STATIC
        src/world.h src/world.cpp
        src/update.h src/update.cpp)
target_link_libraries(survivor_core PUBLIC raylib)   # PUBLIC: world.h включает raylib.h
target_include_directories(survivor_core PUBLIC src)
target_compile_options(survivor_core PRIVATE ${SURVIVOR_WARNINGS})

# --- игра ---
add_executable(survivor
        src/main.cpp
        src/draw.h src/draw.cpp)
target_link_libraries(survivor PRIVATE survivor_core raylib)
target_compile_options(survivor PRIVATE ${SURVIVOR_WARNINGS})

# --- тесты ---
add_executable(tests tests/tests.cpp)
target_link_libraries(tests PRIVATE survivor_core Catch2::Catch2WithMain)
target_compile_options(tests PRIVATE ${SURVIVOR_WARNINGS})

enable_testing()
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)   # там лежит Catch.cmake
include(Catch)
catch_discover_tests(tests)   # каждый TEST_CASE — отдельный тест в ctest