# sdl3_opengl_node2d

# License: MIT

# Status:
- work in progress.

# Project Overview

sdl3_opengl_node2d is a sample project demonstrating a 2D node-based editor using SDL3, SDL_ttf, and FreeType. It renders a graphical interface with the "Kenney Mini" font and supports interactive node manipulation. Built for Windows using the MSYS2/MinGW-w64 environment, it serves as a foundation for developing node-based graphical applications, such as visual scripting or dataflow editors. The project uses SDL3’s software rendering for simplicity and portability.

```text
sdl3_opengl_node2d/
├── CMakeLists.txt
├── Kenney Mini.ttf
├── src/
│   └── main.c
├── run.bat
```

# Features

- Node2D Editor:
    - [x] Dragging: Drag nodes by left-clicking and moving the mouse.
    - [x] Input/Output Ports: Green squares for input ports, red squares for output ports, with text labels (e.g., “In1”, “Out1”).
    - [x] Connections:
        - [x] Left-click a red output square, then a green input square to connect with a white line.
        - [x] Middle-click a disconnect a specific connection.            
    - [x] Node Addition: Right-click away from green squares to add a new node at the click position.
    - [x] delete. drag and delete key.
    - [x] Panning: Middle-click hold and drag to pan the view.
    - [x] Zooming: Scroll wheel to zoom in/out (0.5x to 2.0x), centered on the mouse cursor.
    - [x] Grid g key to toggle snap 
    - [ ] Menus: Planned for node type selection and configuration.
    - [ ] Variables: Planned for node data storage.
    - [ ] Functions: Planned for node-based logic or scripting.
- Rendering:
    - [ ] Nodes rendered as blue 100x100 squares, scaling with zoom.
    - [ ] Red debug rectangle outlines nodes during dragging.
    - [ ] Yellow lines indicate active connections during creation.
- Static Linking: SDL3, SDL_ttf, and FreeType are statically linked to avoid DLL dependencies.
- Cross-Platform Build: Configured with CMake for MinGW-w64, adaptable to other platforms.

    Does not have variables and functions. Just a simple node 2D with the connectors. 


# Goals
- Provide a minimal, working example of SDL3 and SDL_ttf for interactive 2D graphics.
- Serve as a extensible base for a node-based editor (e.g., visual scripting, shader graphs).
- Ensure an easy setup for Windows developers using MSYS2, with clear build instructions.
- Expand functionality with menus, variables, and functions for a fully-featured editor.

# Requirements

## Software

- Operating System: Windows 10 or later (64-bit).
- MSYS2: Software distribution and build platform for Windows.
    - Download: [MSYS2 Installer](https://www.msys2.org/)
- CMake: Version 3.10 or later (install via MSYS2).
- MinGW-w64 Toolchain: Provides GCC, G++, and build tools (install via MSYS2).
- Git: Optional, for cloning the repository.
- Font File: “Kenney Mini.ttf” (included in the project directory).

## Dependencies

The following libraries are fetched and built via CMake’s FetchContent:
- SDL3: Version 3.2.14 (window and graphics handling).
- SDL_ttf: Version 3.2.2 (font rendering for SDL3).
- FreeType: Version 2.13.3 (font engine for SDL_ttf).
- Font: “Kenney Mini.ttf” (must be in the project root).

## MSYS2 Packages

Install these packages in the MSYS2 MinGW 64-bit terminal:

bash

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-g++ mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-pkgconf
```

Setup Instructions
1. Install MSYS2:
    - Download and install MSYS2 from [msys2.org](https://www.msys2.org/).
    - Open the MSYS2 MinGW 64-bit terminal (C:\msys64\mingw64.exe).
2. Install Dependencies:
    
    bash
    ```bash
    pacman -Syu
    ```
    update system.

    bash
    ```
    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-g++ mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-pkgconf
    ```
    
3. Clone or Download the Project:
    - Clone with Git (if available):
        
        bash
        ```bash
        git clone <repository-url>
        cd sdl3_opengl_node2d
        ```
        
    - Or download and extract the project ZIP, then navigate to the directory in the MSYS2 terminal.
        
4. Ensure Font File:
    - Verify that Kenney Mini.ttf is in the project root directory (sdl3_opengl_node2d/Kenney Mini.ttf).
        
5. Build the Project:
    - Create a build directory and run CMake:
        
        bash
        ```bash
        mkdir build
        cd build
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
        cmake --build . --config Debug
        ```
        
    - Alternatively, use the provided run.bat (ensure MSYS2’s mingw64/bin is in your PATH):
        
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
        
    - This generates the executable in build/src/sdl3_node2d_editor.exe.
        
6. Run the Application:
    - From the build directory:
        
        bash
        ```bash
        build/sdl3_node2d_editor.exe
        ```
        
    - Or double-click run.bat if it’s configured to run the executable.
        

# Usage

- Dragging: Left-click and drag a node to move it.
- Connections:
    - Left-click a red output square, then a green input square to create a white connection line.
    - Right-click a green input square to disconnect its connection.
- Node Addition: Right-click away from green squares to add a new node.
- Panning: Middle-click and drag to pan the view.
- Zooming: Scroll wheel to zoom in/out (0.5x to 2.0x).
- Debugging: Console logs show drag positions, connections, disconnections, node additions, and zoom levels.

# Troubleshooting

- Font Not Found:
    - Ensure Kenney Mini.ttf is in the project root. If missing, download from [Kenney Fonts](https://kenney.nl/assets/kenney-fonts).
- Build Errors:
    - Verify MSYS2 packages are installed.
    - Check CMake output for missing dependencies.
    - Ensure run.bat points to the correct MSYS2 path (C:\msys64\mingw64\bin).
- Rendering Issues:
    - If nodes or lines don’t scale/pan, verify the SDL3 version (3.2.14).
    - Test with software rendering:
    
        c
        ```c
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        ```
        
        (Add before SDL_CreateWindowAndRenderer in main.c.)
        
- Performance:
    - Reduce MAX_NODES or MAX_CONNECTIONS in main.c if rendering lags.
        

# Future Roadmap
- Menus: Implement a right-click menu UI for selecting node types and properties.
- Variables: Add support for nodes to store and manipulate data (e.g., integers, floats).
- Functions: Enable nodes to define and execute logic (e.g., math operations, conditionals).
- Node Types: Support different node categories (e.g., math, logic, input/output).
- Save/Load: Allow saving and loading node graphs to/from files.
- OpenGL Integration: Optionally switch to OpenGL for enhanced rendering (if needed).

# Credits:
- blender Node 2D editor idea.
- https://kenney.nl/assets/kenney-fonts
- Grok
    - work code
    - manual feed data and mistakes.