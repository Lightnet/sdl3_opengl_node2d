#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_NODES 1005
#define MAX_CONNECTIONS 40
#define HEADER_HEIGHT 24.0f
#define SLOT_RADIUS 8.0f
#define DISCONNECT_DISTANCE 5.0f
#define OUTLINE_RADIUS 10.0f
#define BORDER_OFFSET 2.0f
#define ZOOM_MIN 0.5f
#define ZOOM_MAX 2.0f
#define ZOOM_STEP 0.1f
#define GRID_SIZE 20.0f

typedef struct {
    float x, y;
    float scale;
} Camera;

typedef struct {
    float x, y;
    float width, height;
    char name[32];
    float inputX, inputY;
    float outputX, outputY;
} Node2D;

typedef struct {
    int fromNode;
    int toNode;
} Connection;

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\n";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D texture1;\n"
    "uniform int useTexture;\n"
    "uniform vec3 color;\n"
    "uniform int isCircle;\n"
    "void main() {\n"
    "   if (isCircle == 1) {\n"
    "       float dist = length(TexCoord - vec2(0.5, 0.5));\n"
    "       if (dist > 0.5) discard;\n"
    "       FragColor = vec4(color, 1.0);\n"
    "   } else if (useTexture == 1) {\n"
    "       FragColor = texture(texture1, TexCoord);\n"
    "   } else {\n"
    "       FragColor = vec4(color, 1.0);\n"
    "   }\n"
    "}\n";

int main(int argc, char* argv[]) {
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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window* window = SDL_CreateWindow("Node2D Editor", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        printf("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        printf("Failed to load GLAD: %s\n", SDL_GetError());
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        getchar();
        return 1;
    }

    printf("GL_VERSION : %s\n", glGetString(GL_VERSION));
    printf("GL_VENDOR  : %s\n", glGetString(GL_VENDOR));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

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
        return 1;
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
        return 1;
    }

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
        return 1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    Camera camera = {0.0f, 0.0f, 1.0f};

    Node2D nodes[MAX_NODES];
    int nodeCount = 3;
    for (int i = 0; i < nodeCount; i++) {
        nodes[i].x = 100.0f + 150.0f * i;
        nodes[i].y = 100.0f;
        nodes[i].width = 100.0f;
        nodes[i].height = 100.0f;
        snprintf(nodes[i].name, sizeof(nodes[i].name), "Node %d", i);
        nodes[i].inputX = nodes[i].x;
        nodes[i].inputY = nodes[i].y + HEADER_HEIGHT + (nodes[i].height - HEADER_HEIGHT) / 2;
        nodes[i].outputX = nodes[i].x + nodes[i].width;
        nodes[i].outputY = nodes[i].inputY;
    }

    Connection connections[MAX_CONNECTIONS];
    int connectionCount = 0;

    TTF_Font* font = TTF_OpenFont("Kenney Mini.ttf", 24);
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

    GLuint textTextures[MAX_NODES];
    float textWidths[MAX_NODES], textHeights[MAX_NODES];
    glGenTextures(nodeCount, textTextures);
    for (int i = 0; i < nodeCount; i++) {
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, nodes[i].name, strlen(nodes[i].name), textColor);
        if (!textSurface) {
            printf("Failed to render text for %s: %s\n", nodes[i].name, SDL_GetError());
            for (int j = 0; j < i; j++) glDeleteTextures(1, &textTextures[j]);
            TTF_CloseFont(font);
            glDeleteProgram(shaderProgram);
            SDL_GL_DestroyContext(glContext);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            getchar();
            return 1;
        }

        SDL_Surface* convertedSurface = SDL_ConvertSurface(textSurface, SDL_PIXELFORMAT_RGBA32);
        if (!convertedSurface) {
            printf("Failed to convert surface for %s: %s\n", nodes[i].name, SDL_GetError());
            SDL_DestroySurface(textSurface);
            for (int j = 0; j < i; j++) glDeleteTextures(1, &textTextures[j]);
            TTF_CloseFont(font);
            glDeleteProgram(shaderProgram);
            SDL_GL_DestroyContext(glContext);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            getchar();
            return 1;
        }

        textWidths[i] = (float)convertedSurface->w;
        textHeights[i] = (float)convertedSurface->h;

        glBindTexture(GL_TEXTURE_2D, textTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convertedSurface->w, convertedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedSurface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        SDL_DestroySurface(convertedSurface);
        SDL_DestroySurface(textSurface);
    }

    GLuint cameraTextTexture = 0;
    float cameraTextWidth = 0, cameraTextHeight = 0;
    bool updateCameraText = true;

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    GLint isCircleLoc = glGetUniformLocation(shaderProgram, "isCircle");

    int draggedNode = -1;
    float dragOffsetX, dragOffsetY;
    int connectingNode = -1;
    float connectStartX, connectStartY;
    bool panning = false;
    float panStartX, panStartY;
    bool gridSnapping = true; // New: Grid snapping toggle

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_DELETE) {
                    if (draggedNode != -1 && nodeCount > 0) {
                        printf("Deleted %s\n", nodes[draggedNode].name);
                        glDeleteTextures(1, &textTextures[draggedNode]);
                        for (int i = draggedNode; i < nodeCount - 1; i++) {
                            nodes[i] = nodes[i + 1];
                            textTextures[i] = textTextures[i + 1];
                            textWidths[i] = textWidths[i + 1];
                            textHeights[i] = textHeights[i + 1];
                        }
                        int i = 0;
                        while (i < connectionCount) {
                            if (connections[i].fromNode == draggedNode || connections[i].toNode == draggedNode) {
                                for (int j = i; j < connectionCount - 1; j++) {
                                    connections[j] = connections[j + 1];
                                }
                                connectionCount--;
                            } else {
                                if (connections[i].fromNode > draggedNode) connections[i].fromNode--;
                                if (connections[i].toNode > draggedNode) connections[i].toNode--;
                                i++;
                            }
                        }
                        nodeCount--;
                        draggedNode = -1;
                        updateCameraText = true;
                    }
                }
                else if (event.key.key == SDLK_PLUS || event.key.key == SDLK_EQUALS) {
                    float mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    float worldX = (mouseX + camera.x) / camera.scale;
                    float worldY = (mouseY + camera.y) / camera.scale;
                    float oldScale = camera.scale;
                    camera.scale = fminf(camera.scale + ZOOM_STEP, ZOOM_MAX);
                    if (camera.scale != oldScale) {
                        camera.x = worldX * camera.scale - mouseX;
                        camera.y = worldY * camera.scale - mouseY;
                        printf("Zoomed to scale %.2f\n", camera.scale);
                        updateCameraText = true;
                    }
                }
                else if (event.key.key == SDLK_MINUS) {
                    float mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    float worldX = (mouseX + camera.x) / camera.scale;
                    float worldY = (mouseY + camera.y) / camera.scale;
                    float oldScale = camera.scale;
                    camera.scale = fmaxf(camera.scale - ZOOM_STEP, ZOOM_MIN);
                    if (camera.scale != oldScale) {
                        camera.x = worldX * camera.scale - mouseX;
                        camera.y = worldY * camera.scale - mouseY;
                        printf("Zoomed to scale %.2f\n", camera.scale);
                        updateCameraText = true;
                    }
                }
                else if (event.key.key == SDLK_G) { // New: Toggle grid snapping
                    gridSnapping = !gridSnapping;
                    printf("Grid snapping %s\n", gridSnapping ? "enabled" : "disabled");
                    updateCameraText = true;
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                float mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                float worldX = (mouseX + camera.x) / camera.scale;
                float worldY = (mouseY + camera.y) / camera.scale;
                float oldScale = camera.scale;
                if (event.wheel.y > 0) {
                    camera.scale = fminf(camera.scale + ZOOM_STEP, ZOOM_MAX);
                } else if (event.wheel.y < 0) {
                    camera.scale = fmaxf(camera.scale - ZOOM_STEP, ZOOM_MIN);
                }
                if (camera.scale != oldScale) {
                    camera.x = worldX * camera.scale - mouseX;
                    camera.y = worldY * camera.scale - mouseY;
                    printf("Zoomed to scale %.2f\n", camera.scale);
                    updateCameraText = true;
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mouseX = event.button.x;
                float mouseY = event.button.y;
                float worldX = (mouseX + camera.x) / camera.scale;
                float worldY = (mouseY + camera.y) / camera.scale;

                if (event.button.button == SDL_BUTTON_LEFT) {
                    for (int i = 0; i < nodeCount; i++) {
                        float dx = worldX - nodes[i].outputX;
                        float dy = worldY - nodes[i].outputY;
                        if (dx * dx + dy * dy <= SLOT_RADIUS * SLOT_RADIUS / (camera.scale * camera.scale)) {
                            connectingNode = i;
                            connectStartX = nodes[i].outputX;
                            connectStartY = nodes[i].outputY;
                            printf("Starting connection from %s\n", nodes[i].name);
                            break;
                        }
                    }

                    if (connectingNode == -1) {
                        for (int i = nodeCount - 1; i >= 0; i--) {
                            if (worldX >= nodes[i].x && worldX <= nodes[i].x + nodes[i].width &&
                                worldY >= nodes[i].y && worldY <= nodes[i].y + HEADER_HEIGHT) {
                                draggedNode = i;
                                dragOffsetX = worldX - nodes[i].x;
                                dragOffsetY = worldY - nodes[i].y;
                                printf("Dragging %s at (%.0f, %.0f)\n", nodes[i].name, nodes[i].x, nodes[i].y);
                                nodes[i].inputX = nodes[i].x;
                                nodes[i].inputY = nodes[i].y + HEADER_HEIGHT + (nodes[i].height - HEADER_HEIGHT) / 2;
                                nodes[i].outputX = nodes[i].x + nodes[i].width;
                                nodes[i].outputY = nodes[i].inputY;
                                break;
                            }
                        }
                    }
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (nodeCount < MAX_NODES) {
                        nodes[nodeCount].x = gridSnapping ? roundf(worldX / GRID_SIZE) * GRID_SIZE : worldX;
                        nodes[nodeCount].y = gridSnapping ? roundf(worldY / GRID_SIZE) * GRID_SIZE : worldY;
                        nodes[nodeCount].width = 100.0f;
                        nodes[nodeCount].height = 100.0f;
                        snprintf(nodes[nodeCount].name, sizeof(nodes[nodeCount].name), "Node %d", nodeCount);
                        nodes[nodeCount].inputX = nodes[nodeCount].x;
                        nodes[nodeCount].inputY = nodes[nodeCount].y + HEADER_HEIGHT + (nodes[nodeCount].height - HEADER_HEIGHT) / 2;
                        nodes[nodeCount].outputX = nodes[nodeCount].x + nodes[nodeCount].width;
                        nodes[nodeCount].outputY = nodes[nodeCount].inputY;

                        SDL_Color textColor = {255, 255, 255, 255};
                        SDL_Surface* textSurface = TTF_RenderText_Blended(font, nodes[nodeCount].name, strlen(nodes[nodeCount].name), textColor);
                        if (!textSurface) {
                            printf("Failed to render text for %s: %s\n", nodes[nodeCount].name, SDL_GetError());
                        } else {
                            SDL_Surface* convertedSurface = SDL_ConvertSurface(textSurface, SDL_PIXELFORMAT_RGBA32);
                            if (!convertedSurface) {
                                printf("Failed to convert surface for %s: %s\n", nodes[nodeCount].name, SDL_GetError());
                                SDL_DestroySurface(textSurface);
                            } else {
                                glGenTextures(1, &textTextures[nodeCount]);
                                glBindTexture(GL_TEXTURE_2D, textTextures[nodeCount]);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convertedSurface->w, convertedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedSurface->pixels);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                                textWidths[nodeCount] = (float)convertedSurface->w;
                                textHeights[nodeCount] = (float)convertedSurface->h;

                                SDL_DestroySurface(convertedSurface);
                                SDL_DestroySurface(textSurface);

                                printf("Added %s at (%.0f, %.0f)\n", nodes[nodeCount].name, nodes[nodeCount].x, nodes[nodeCount].y);
                                nodeCount++;
                                updateCameraText = true;
                            }
                        }
                    } else {
                        printf("Cannot add node: Maximum node count (%d) reached\n", MAX_NODES);
                    }
                }
                else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    panning = true;
                    panStartX = mouseX;
                    panStartY = mouseY;
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (connectingNode != -1) {
                        float mouseX = event.button.x;
                        float mouseY = event.button.y;
                        float worldX = (mouseX + camera.x) / camera.scale;
                        float worldY = (mouseY + camera.y) / camera.scale;
                        for (int i = 0; i < nodeCount; i++) {
                            if (i == connectingNode) continue;
                            float dx = worldX - nodes[i].inputX;
                            float dy = worldY - nodes[i].inputY;
                            if (dx * dx + dy * dy <= SLOT_RADIUS * SLOT_RADIUS / (camera.scale * camera.scale)) {
                                bool inputUsed = false;
                                for (int j = 0; j < connectionCount; j++) {
                                    if (connections[j].toNode == i) {
                                        inputUsed = true;
                                        break;
                                    }
                                }
                                if (!inputUsed && connectionCount < MAX_CONNECTIONS) {
                                    connections[connectionCount].fromNode = connectingNode;
                                    connections[connectionCount].toNode = i;
                                    connectionCount++;
                                    printf("Connected %s to %s\n", nodes[connectingNode].name, nodes[i].name);
                                    updateCameraText = true;
                                }
                                break;
                            }
                        }
                        connectingNode = -1;
                    }
                    if (draggedNode != -1) {
                        printf("Dropped %s at (%.0f, %.0f)\n", nodes[draggedNode].name, nodes[draggedNode].x, nodes[draggedNode].y);
                        draggedNode = -1;
                    }
                }
                else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    if (panning) {
                        float mouseX = event.button.x;
                        float mouseY = event.button.y;
                        if (fabs(mouseX - panStartX) < 2 && fabs(mouseY - panStartY) < 2) {
                            float worldX = (mouseX + camera.x) / camera.scale;
                            float worldY = (mouseY + camera.y) / camera.scale;
                            for (int i = 0; i < connectionCount; i++) {
                                float x1 = nodes[connections[i].fromNode].outputX;
                                float y1 = nodes[connections[i].fromNode].outputY;
                                float x2 = nodes[connections[i].toNode].inputX;
                                float y2 = nodes[connections[i].toNode].inputY;

                                float dx = x2 - x1;
                                float dy = y2 - y1;
                                float len_sq = dx * dx + dy * dy;
                                if (len_sq == 0) continue;

                                float t = ((worldX - x1) * dx + (worldY - y1) * dy) / len_sq;
                                t = fmaxf(0, fminf(1, t));
                                float projX = x1 + t * dx;
                                float projY = y1 + t * dy;
                                float dist = sqrtf((worldX - projX) * (worldX - projX) + (worldY - projY) * (worldY - projY));

                                if (dist <= DISCONNECT_DISTANCE / camera.scale) {
                                    printf("Disconnected %s from %s\n", nodes[connections[i].fromNode].name, nodes[connections[i].toNode].name);
                                    for (int j = i; j < connectionCount - 1; j++) {
                                        connections[j] = connections[j + 1];
                                    }
                                    connectionCount--;
                                    i--;
                                    updateCameraText = true;
                                }
                            }
                        }
                        panning = false;
                        printf("Panned to (%.2f, %.2f)\n", camera.x, camera.y);
                        updateCameraText = true;
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (draggedNode != -1) {
                    float mouseX = event.motion.x;
                    float mouseY = event.motion.y;
                    float worldX = (mouseX + camera.x) / camera.scale;
                    float worldY = (mouseY + camera.y) / camera.scale;
                    nodes[draggedNode].x = gridSnapping ? roundf((worldX - dragOffsetX) / GRID_SIZE) * GRID_SIZE : worldX - dragOffsetX;
                    nodes[draggedNode].y = gridSnapping ? roundf((worldY - dragOffsetY) / GRID_SIZE) * GRID_SIZE : worldY - dragOffsetY;
                    nodes[draggedNode].inputX = nodes[draggedNode].x;
                    nodes[draggedNode].inputY = nodes[draggedNode].y + HEADER_HEIGHT + (nodes[draggedNode].height - HEADER_HEIGHT) / 2;
                    nodes[draggedNode].outputX = nodes[draggedNode].x + nodes[draggedNode].width;
                    nodes[draggedNode].outputY = nodes[draggedNode].inputY;
                    updateCameraText = true;
                }
                else if (panning) {
                    float mouseX = event.motion.x;
                    float mouseY = event.motion.y;
                    camera.x -= (mouseX - panStartX);
                    camera.y -= (mouseY - panStartY);
                    panStartX = mouseX;
                    panStartY = mouseY;
                    updateCameraText = true;
                }
            }
        }

        if (updateCameraText) {
            if (cameraTextTexture) glDeleteTextures(1, &cameraTextTexture);
            cameraTextTexture = 0;
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Camera: (%.0f, %.0f) Zoom: %.2f Snap: %s", camera.x, camera.y, camera.scale, gridSnapping ? "ON" : "OFF");
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, buffer, strlen(buffer), textColor);
            if (textSurface) {
                SDL_Surface* convertedSurface = SDL_ConvertSurface(textSurface, SDL_PIXELFORMAT_RGBA32);
                if (convertedSurface) {
                    glGenTextures(1, &cameraTextTexture);
                    glBindTexture(GL_TEXTURE_2D, cameraTextTexture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, convertedSurface->w, convertedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, convertedSurface->pixels);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    cameraTextWidth = (float)convertedSurface->w;
                    cameraTextHeight = (float)convertedSurface->h;

                    SDL_DestroySurface(convertedSurface);
                }
                SDL_DestroySurface(textSurface);
            }
            updateCameraText = false;
        }

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        if (connectionCount > 0 || connectingNode != -1) {
            float lineVertices[4 * 4];
            glUniform1i(useTextureLoc, 0);
            glUniform1i(isCircleLoc, 0);
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

            for (int i = 0; i < connectionCount; i++) {
                float x1 = (nodes[connections[i].fromNode].outputX * camera.scale - camera.x) / WINDOW_WIDTH * 2 - 1;
                float y1 = 1 - ((nodes[connections[i].fromNode].outputY * camera.scale - camera.y) / WINDOW_HEIGHT * 2);
                float x2 = (nodes[connections[i].toNode].inputX * camera.scale - camera.x) / WINDOW_WIDTH * 2 - 1;
                float y2 = 1 - ((nodes[connections[i].toNode].inputY * camera.scale - camera.y) / WINDOW_HEIGHT * 2);
                lineVertices[0] = x1;
                lineVertices[1] = y1;
                lineVertices[2] = 0.0f;
                lineVertices[3] = 0.0f;
                lineVertices[4] = x2;
                lineVertices[5] = y2;
                lineVertices[6] = 0.0f;
                lineVertices[7] = 0.0f;

                glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
                glDrawArrays(GL_LINES, 0, 2);
            }

            if (connectingNode != -1) {
                float mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                float x1 = (connectStartX * camera.scale - camera.x) / WINDOW_WIDTH * 2 - 1;
                float y1 = 1 - ((connectStartY * camera.scale - camera.y) / WINDOW_HEIGHT * 2);
                float x2 = mouseX / WINDOW_WIDTH * 2 - 1;
                float y2 = 1 - (mouseY / WINDOW_HEIGHT * 2);
                lineVertices[0] = x1;
                lineVertices[1] = y1;
                lineVertices[2] = 0.0f;
                lineVertices[3] = 0.0f;
                lineVertices[4] = x2;
                lineVertices[5] = y2;
                lineVertices[6] = 0.0f;
                lineVertices[7] = 0.0f;

                glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
                glDrawArrays(GL_LINES, 0, 2);
            }
        }

        for (int i = 0; i < nodeCount; i++) {
            float scaledWidth = nodes[i].width * camera.scale;
            float scaledHeight = nodes[i].height * camera.scale;
            float scaledX = nodes[i].x * camera.scale - camera.x;
            float scaledY = nodes[i].y * camera.scale - camera.y;
            float bodyVertices[] = {
                scaledX / WINDOW_WIDTH * 2 - 1, 1 - scaledY / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                scaledX / WINDOW_WIDTH * 2 - 1, 1 - (scaledY + scaledHeight) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledX + scaledWidth) / WINDOW_WIDTH * 2 - 1, 1 - (scaledY + scaledHeight) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledX + scaledWidth) / WINDOW_WIDTH * 2 - 1, 1 - scaledY / WINDOW_HEIGHT * 2, 0.0f, 0.0f
            };
            unsigned int indices[] = {0, 1, 2, 2, 3, 0};

            glBufferData(GL_ARRAY_BUFFER, sizeof(bodyVertices), bodyVertices, GL_STATIC_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glUniform1i(useTextureLoc, 0);
            glUniform1i(isCircleLoc, 0);
            glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            float scaledHeaderHeight = HEADER_HEIGHT * camera.scale;
            float headerVertices[] = {
                scaledX / WINDOW_WIDTH * 2 - 1, 1 - scaledY / WINDOW_HEIGHT * 2, 0.0f, 0.0f, // Fixed: WINDOW_HEIGHT
                scaledX / WINDOW_WIDTH * 2 - 1, 1 - (scaledY + scaledHeaderHeight) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledX + scaledWidth) / WINDOW_WIDTH * 2 - 1, 1 - (scaledY + scaledHeaderHeight) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledX + scaledWidth) / WINDOW_WIDTH * 2 - 1, 1 - scaledY / WINDOW_HEIGHT * 2, 0.0f, 0.0f
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(headerVertices), headerVertices, GL_STATIC_DRAW);
            glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            float scaledSlotRadius = SLOT_RADIUS * camera.scale;
            float scaledSlotSize = scaledSlotRadius * 2;
            float scaledInputX = nodes[i].inputX * camera.scale - camera.x;
            float scaledInputY = nodes[i].inputY * camera.scale - camera.y;
            float inputVertices[] = {
                (scaledInputX - scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY - scaledSlotRadius) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledInputX - scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY + scaledSlotRadius) / WINDOW_HEIGHT * 2, 0.0f, 1.0f,
                (scaledInputX + scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY + scaledSlotRadius) / WINDOW_HEIGHT * 2, 1.0f, 1.0f,
                (scaledInputX + scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY - scaledSlotRadius) / WINDOW_HEIGHT * 2, 1.0f, 0.0f
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(inputVertices), inputVertices, GL_STATIC_DRAW);
            glUniform1i(useTextureLoc, 0);
            glUniform1i(isCircleLoc, 1);
            glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            float scaledOutputX = nodes[i].outputX * camera.scale - camera.x;
            float scaledOutputY = nodes[i].outputY * camera.scale - camera.y;
            float outputVertices[] = {
                (scaledOutputX - scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledOutputY - scaledSlotRadius) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledOutputX - scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledOutputY + scaledSlotRadius) / WINDOW_HEIGHT * 2, 0.0f, 1.0f,
                (scaledOutputX + scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledOutputY + scaledSlotRadius) / WINDOW_HEIGHT * 2, 1.0f, 1.0f,
                (scaledOutputX + scaledSlotRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledOutputY - scaledSlotRadius) / WINDOW_HEIGHT * 2, 1.0f, 0.0f
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(outputVertices), outputVertices, GL_STATIC_DRAW);
            glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            float scaledTextX = (nodes[i].x + 5) * camera.scale - camera.x;
            float scaledTextY = (nodes[i].y + 4) * camera.scale - camera.y;
            float scaledTextWidth = textWidths[i] * camera.scale;
            float scaledTextHeight = textHeights[i] * camera.scale;
            float textVertices[] = {
                scaledTextX / WINDOW_WIDTH * 2 - 1, 1 - scaledTextY / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                scaledTextX / WINDOW_WIDTH * 2 - 1, 1 - (scaledTextY + scaledTextHeight) / WINDOW_HEIGHT * 2, 0.0f, 1.0f,
                (scaledTextX + scaledTextWidth) / WINDOW_WIDTH * 2 - 1, 1 - (scaledTextY + scaledTextHeight) / WINDOW_HEIGHT * 2, 1.0f, 1.0f,
                (scaledTextX + scaledTextWidth) / WINDOW_WIDTH * 2 - 1, 1 - scaledTextY / WINDOW_HEIGHT * 2, 1.0f, 0.0f
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(textVertices), textVertices, GL_STATIC_DRAW);
            glUniform1i(useTextureLoc, 1);
            glUniform1i(isCircleLoc, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textTextures[i]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            if (i == draggedNode) {
                float scaledBorderOffset = BORDER_OFFSET * camera.scale;
                float borderVertices[] = {
                    (scaledX - scaledBorderOffset) / WINDOW_WIDTH * 2 - 1, 1 - (scaledY - scaledBorderOffset) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                    (scaledX - scaledBorderOffset) / WINDOW_WIDTH * 2 - 1, 1 - (scaledY + scaledHeight + scaledBorderOffset) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                    (scaledX + scaledWidth + scaledBorderOffset) / WINDOW_WIDTH * 2 - 1, 1 - (scaledY + scaledHeight + scaledBorderOffset) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                    (scaledX + scaledWidth + scaledBorderOffset) / WINDOW_WIDTH * 2 - 1, 1 - (scaledY - scaledBorderOffset) / WINDOW_HEIGHT * 2, 0.0f, 0.0f
                };
                glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices, GL_STATIC_DRAW);
                glUniform1i(useTextureLoc, 0);
                glUniform1i(isCircleLoc, 0);
                glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
        }

        if (connectingNode != -1) {
            float scaledOutlineRadius = OUTLINE_RADIUS * camera.scale;
            float outlineSize = scaledOutlineRadius * 2;
            float scaledConnectX = connectStartX * camera.scale - camera.x;
            float scaledConnectY = connectStartY * camera.scale - camera.y;
            float outputOutlineVertices[] = {
                (scaledConnectX - scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledConnectY - scaledOutlineRadius) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                (scaledConnectX - scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledConnectY + scaledOutlineRadius) / WINDOW_HEIGHT * 2, 0.0f, 1.0f,
                (scaledConnectX + scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledConnectY + scaledOutlineRadius) / WINDOW_HEIGHT * 2, 1.0f, 1.0f,
                (scaledConnectX + scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledConnectY - scaledOutlineRadius) / WINDOW_HEIGHT * 2, 1.0f, 0.0f
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(outputOutlineVertices), outputOutlineVertices, GL_STATIC_DRAW);
            glUniform1i(useTextureLoc, 0);
            glUniform1i(isCircleLoc, 1);
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            float mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            float worldX = (mouseX + camera.x) / camera.scale;
            float worldY = (mouseY + camera.y) / camera.scale;
            for (int i = 0; i < nodeCount; i++) {
                if (i == connectingNode) continue;
                float dx = worldX - nodes[i].inputX;
                float dy = worldY - nodes[i].inputY;
                if (dx * dx + dy * dy <= SLOT_RADIUS * SLOT_RADIUS / (camera.scale * camera.scale)) {
                    float scaledInputX = nodes[i].inputX * camera.scale - camera.x;
                    float scaledInputY = nodes[i].inputY * camera.scale - camera.y;
                    float inputOutlineVertices[] = {
                        (scaledInputX - scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY - scaledOutlineRadius) / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                        (scaledInputX - scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY + scaledOutlineRadius) / WINDOW_HEIGHT * 2, 0.0f, 1.0f,
                        (scaledInputX + scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY + scaledOutlineRadius) / WINDOW_HEIGHT * 2, 1.0f, 1.0f,
                        (scaledInputX + scaledOutlineRadius) / WINDOW_WIDTH * 2 - 1, 1 - (scaledInputY - scaledOutlineRadius) / WINDOW_HEIGHT * 2, 1.0f, 0.0f
                    };
                    glBufferData(GL_ARRAY_BUFFER, sizeof(inputOutlineVertices), inputOutlineVertices, GL_STATIC_DRAW);
                    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
        }

        if (cameraTextTexture) {
            float textVertices[] = {
                10.0f / WINDOW_WIDTH * 2 - 1, 1 - 10.0f / WINDOW_HEIGHT * 2, 0.0f, 0.0f,
                10.0f / WINDOW_WIDTH * 2 - 1, 1 - (10.0f + cameraTextHeight) / WINDOW_HEIGHT * 2, 0.0f, 1.0f,
                (10.0f + cameraTextWidth) / WINDOW_WIDTH * 2 - 1, 1 - (10.0f + cameraTextHeight) / WINDOW_HEIGHT * 2, 1.0f, 1.0f,
                (10.0f + cameraTextWidth) / WINDOW_WIDTH * 2 - 1, 1 - 10.0f / WINDOW_HEIGHT * 2, 1.0f, 0.0f
            };
            glBufferData(GL_ARRAY_BUFFER, sizeof(textVertices), textVertices, GL_STATIC_DRAW);
            glUniform1i(useTextureLoc, 1);
            glUniform1i(isCircleLoc, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cameraTextTexture);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        SDL_GL_SwapWindow(window);
    }

    for (int i = 0; i < nodeCount; i++) {
        glDeleteTextures(1, &textTextures[i]);
    }
    if (cameraTextTexture) glDeleteTextures(1, &cameraTextTexture);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    TTF_CloseFont(font);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}