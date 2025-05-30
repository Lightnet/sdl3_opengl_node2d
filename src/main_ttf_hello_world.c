#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
// #include <SDL3/SDL_opengl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Vertex shader
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\n";

// Fragment shader
const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D texture1;\n"
    "void main() {\n"
    "   FragColor = texture(texture1, TexCoord);\n"
    "}\n";

int main(int argc, char* argv[]) {
    // Initialize SDL3 and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        getchar();
        return 1;
    }
    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", SDL_GetError());
        SDL_Quit();
        getchar();
        return 1;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create window
    SDL_Window* window = SDL_CreateWindow("SDL3 Text Rendering with OpenGL", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Make context current
    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        printf("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Initialize GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        printf("Failed to initialize GLAD: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Print OpenGL info
    printf("GL_VERSION : %s\n", glGetString(GL_VERSION));
    printf("GL_VENDOR  : %s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    // Set viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("Vertex shader compilation failed: %s\n", infoLog);
        getchar();
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("Fragment shader compilation failed: %s\n", infoLog);
        getchar();
    }

    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Shader program linking failed: %s\n", infoLog);
        getchar();
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Load font
    TTF_Font* font = TTF_OpenFont("Kenney Mini.ttf", 48);
    if (!font) {
        printf("Failed to load font: %s\n", SDL_GetError());
        glDeleteProgram(shaderProgram);
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Render text to surface
    SDL_Color textColor = {255, 255, 255, 255}; // White text
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, "Hello, World!", strlen("Hello, World!"), textColor);
    if (!textSurface) {
        printf("Failed to render text: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        glDeleteProgram(shaderProgram);
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Verify surface format
    printf("Surface format: %s\n", SDL_GetPixelFormatName(textSurface->format));
    printf("Surface dimensions: %dx%d\n", textSurface->w, textSurface->h);

    // Convert surface to RGBA if necessary
    SDL_Surface* convertedSurface = SDL_ConvertSurface(textSurface, SDL_PIXELFORMAT_RGBA32);
    if (!convertedSurface) {
        printf("Failed to convert surface: %s\n", SDL_GetError());
        SDL_DestroySurface(textSurface);
        TTF_CloseFont(font);
        glDeleteProgram(shaderProgram);
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    // Create OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convertedSurface->w, convertedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedSurface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Get text dimensions and position
    float texW = (float)convertedSurface->w;
    float texH = (float)convertedSurface->h;
    float x = (WINDOW_WIDTH - texW) / 2.0f;
    float y = (WINDOW_HEIGHT - texH) / 2.0f;

    // Define quad vertices (normalized device coordinates)
    float vertices[] = {
        // Positions                        // Texture Coords
        x / WINDOW_WIDTH * 2 - 1, 1 - y / WINDOW_HEIGHT * 2, 0.0f, 0.0f, // Top-left
        x / WINDOW_WIDTH * 2 - 1, 1 - (y + texH) / WINDOW_HEIGHT * 2, 0.0f, 1.0f, // Bottom-left
        (x + texW) / WINDOW_WIDTH * 2 - 1, 1 - (y + texH) / WINDOW_HEIGHT * 2, 1.0f, 1.0f, // Bottom-right
        (x + texW) / WINDOW_WIDTH * 2 - 1, 1 - y / WINDOW_HEIGHT * 2, 1.0f, 0.0f  // Top-right
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Set up vertex objects
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable blending for text transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set texture uniform
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Main loop
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render texture
        glUseProgram(shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    SDL_DestroySurface(convertedSurface);
    SDL_DestroySurface(textSurface);
    TTF_CloseFont(font);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}