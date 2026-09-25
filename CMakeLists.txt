cmake_minimum_required(VERSION 3.25)
project(survivor LANGUAGES C CXX)   # C нужен, чтобы собрать саму raylib

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- raylib 6.0: скачивается и собирается сама при первой загрузке проекта ---
# Нужен интернет при первой конфигурации; Git не нужен (качается архив).
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # примеры raylib не собираем
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM                                    # не показывать предупреждения из заголовков raylib
)
FetchContent_MakeAvailable(raylib)

# --- игра ---
add_executable(survivor main.cpp
        world.h
        world.cpp
        update.h
        update.cpp
        draw.h
        draw.cpp)
target_link_libraries(survivor PRIVATE raylib)

if (MSVC)
    target_compile_options(survivor PRIVATE /W4 /utf-8)
else()
    target_compile_options(survivor PRIVATE -Wall -Wextra -Wpedantic)
endif()
