# mingw64_sdl3_sample

# Project Overview

mingw64_sdl3_sample is a minimal sample project demonstrating the use of SDL3, SDL_ttf, and FreeType to render text in a window using the "Kenney Mini" font. This project is designed for Windows using the MSYS2/MinGW-w64 environment, serving as a testbed for font rendering and SDL3-based applications. It provides a simple foundation for building a terminal emulator or other graphical applications requiring text rendering.

```
mingw64_sdl3_sample/
├── CMakeLists.txt
├── Kenney Mini.ttf
├── src/
│   └── main.c
```

## Features

- Renders text using the "Kenney Mini" font with SDL_ttf.
- Creates a window with SDL3 to display centered text ("Hello, SDL_ttf!").
- Configured for static linking of SDL3, SDL_ttf, and FreeType to avoid DLL dependencies.
- Built using CMake and MinGW-w64 for cross-platform compatibility.

## Goals

- Provide a working example of SDL3 and SDL_ttf integration.
- Serve as a starting point for more complex projects, such as a terminal emulator.
- Ensure a straightforward setup process for Windows developers using MSYS2.

## Requirements

### Software
- Operating System: Windows 10 or later (64-bit).
- MSYS2: A software distribution and building platform for Windows.
    - Download: [MSYS2 Installer](https://www.msys2.org/)
- CMake: Version 3.10 or later (recommended: latest version available via MSYS2).
- MinGW-w64 Toolchain: Provides GCC, G++, and other tools for compiling.
- Git: For cloning the project repository (optional, if using source control).

# Dependencies

The project uses the following libraries, which are automatically fetched and built via CMake’s FetchContent:

- SDL3: Version 3.2.14 (Simple DirectMedia Layer for window and graphics handling).
- SDL_ttf: Version 3.2.2 (Font rendering library for SDL3).
- FreeType: Version 2.13.3 (Font rendering engine used by SDL_ttf).
- Font: "Kenney Mini.ttf" (included in the project directory).

## MSYS2 Packages

Install the following packages in MSYS2 to ensure a complete build environment:

bash
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-g++
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-pkgconf
```

# Setup Instructions

1. Install MSYS2
2. Download and install MSYS2 from [msys2.org](https://www.msys2.org/).
3. Open the MSYS2 MinGW 64-bit terminal (mingw64.exe) from the MSYS2 installation directory (e.g., C:\msys64\mingw64.exe).
4. Update the package database and install required tools:
    
bash
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-g++ mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-pkgconf
```

cmd
```text
@echo off
setlocal
set MSYS2_PATH=C:\msys64\mingw64\bin
set PATH=%MSYS2_PATH%;%PATH%
if not exist build mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=%MSYS2_PATH%\gcc.exe -DCMAKE_CXX_COMPILER=%MSYS2_PATH%\g++.exe
cmake --build . --config Debug
endlocal
```
This config and build application.


# Credits

- Kenney Fonts: The "Kenney Mini.ttf" font is provided by [Kenney](https://kenney.nl/assets/kenney-fonts).
- SDL: [Simple DirectMedia Layer](https://www.libsdl.org/).
- SDL_ttf: [SDL_ttf Library](https://github.com/libsdl-org/SDL_ttf).
- FreeType: [FreeType Project](https://www.freetype.org/).


