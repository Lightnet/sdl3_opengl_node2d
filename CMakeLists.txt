cmake_minimum_required(VERSION 3.22)
project(sdl3_opengl_node2d_editor C)

#set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Verbose output" FORCE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.14
    GIT_SHALLOW TRUE
)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 static library" FORCE) # from cmake SDL
set(SDL_SHARED OFF CACHE BOOL "Build SDL3 shared library" FORCE) # from cmake SDL
set(SDL_TEST OFF CACHE BOOL "Build SDL3 tests" FORCE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
    GIT_SHALLOW TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable BZip2" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable ZLIB" FORCE)
FetchContent_MakeAvailable(freetype)
# set(FREETYPE_LIBRARY ${CMAKE_BINARY_DIR}/_deps/freetype-build/libfreetype.a CACHE PATH "Path to FreeType library" FORCE)
set(FREETYPE_LIBRARY ${CMAKE_BINARY_DIR}/libfreetyped.a CACHE PATH "Path to FreeType library" FORCE)
set(FREETYPE_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/_deps/freetype-src/include CACHE PATH "FreeType include directories" FORCE)

FetchContent_Declare(
    sdl_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
    # GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(sdl_ttf)

# Add GLAD manually
# https://gen.glad.sh/
    # - Generator: c
    # - API: gl=3.3
    # - Profile: Core
    # - Options
    #     - Generate a loader: Checked
add_library(glad STATIC 
    ${CMAKE_SOURCE_DIR}/glad/src/gl.c
)
target_include_directories(glad PUBLIC 
    ${CMAKE_SOURCE_DIR}/glad/include
)

set(APP_NAME sdl3_node2d_editor)

add_executable(${APP_NAME} src/main.c)

# https://github.com/libsdl-org/SDL_ttf/blob/release-3.2.2/CMakeLists.txt
# add_library(SDL3_ttf::SDL3_ttf ALIAS ${sdl3_ttf_target_name})
target_link_libraries(${APP_NAME} PRIVATE 
    # SDL3::SDL3-static
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    freetype
    glad
)

if(WIN32)
    target_link_libraries(${APP_NAME} PRIVATE opengl32)
else()
    target_link_libraries(${APP_NAME} PRIVATE GL)
endif()

target_include_directories(${APP_NAME} PRIVATE 
    ${freetype_SOURCE_DIR}/include
    ${SDL3_SOURCE_DIR}/include
    ${sdl_ttf_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/glad/include
)

set_property(TARGET ${APP_NAME} PROPERTY C_STANDARD 11)

configure_file("Kenney Mini.ttf" "${CMAKE_BINARY_DIR}/Kenney Mini.ttf" COPYONLY)