cmake_minimum_required(VERSION 3.10)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c)

find_package(PkgConfig)

# CAIRO
pkg_check_modules(CAIRO REQUIRED cairo)
target_link_libraries(untitled PRIVATE ${CAIRO_LIBRARIES})
target_include_directories(untitled PUBLIC ${CAIRO_INCLUDE_DIRS})
target_compile_options(untitled PUBLIC ${CAIRO_CFLAGS_OTHER})

# Harfbuzz
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
target_link_libraries(untitled PRIVATE ${HARFBUZZ_LIBRARIES})
target_include_directories(untitled PUBLIC ${HARFBUZZ_INCLUDE_DIRS})
target_compile_options(untitled PUBLIC ${HARFBUZZ_CFLAGS_OTHER})

# Freetype2
pkg_check_modules(FREETYPE REQUIRED freetype2)
target_link_libraries(untitled PRIVATE ${FREETYPE_LIBRARIES})
target_include_directories(untitled PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_compile_options(untitled PUBLIC ${FREETYPE_CFLAGS_OTHER})

# SDL2
pkg_check_modules(SDL2 REQUIRED sdl2)
target_link_libraries(untitled PRIVATE ${SDL2_LIBRARIES})
target_include_directories(untitled PUBLIC ${SDL2_INCLUDE_DIRS})
target_compile_options(untitled PUBLIC ${SDL2_CFLAGS_OTHER})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ex-harfbuzz-sdl2-cairo-freetype-emoji")
