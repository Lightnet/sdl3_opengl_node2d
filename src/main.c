#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/gl.h>
#include <ft2build.h>
#include FT_FREETYPE_H // FreeType header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float x, y; // Center position in NDC
    float width, height; // Size of the main square
    float input_x, input_y; // Center of input rectangle
    float output_x, output_y; // Center of output rectangle
    float io_width, io_height; // Size of input/output rectangles
    GLuint vao, vbo; // For the main square
    GLuint input_vao, input_vbo; // For input rectangle
    GLuint output_vao, output_vbo; // For output rectangle
    int connected_to; // Index of Node2D this node's input is connected to (-1 if none)
    bool is_input_connected; // True if input is connected
    bool is_output_connected; // True if output is connected
    char name[16]; // Name of the node (e.g., "Node 0")
} Node2D;

#define MAX_NODES 10
static Node2D nodes[MAX_NODES]; // Array of Node2D instances
static Node2D node; // Single Node2D 

static int node_count = 2; // Start with 2 nodes for demonstration

static float square_pos_x = 0.0f; // Square's center X in NDC
static float square_pos_y = 0.0f; // Square's center Y in NDC
static bool is_dragging = false; // Track if mouse is dragging

static int dragging_node = -1; // Index of node being dragged

static float drag_offset_x = 0.0f; // Offset from square center to mouse click
static float drag_offset_y = 0.0f; // Offset from square center to mouse click

static bool is_connecting = false; // Track if connecting
static int connecting_node = -1; // Index of node whose input is being connected
static bool is_connecting_from_output = false; // Track if connecting from output (red)
static float connecting_x, connecting_y; // Current mouse position for drawing line
static GLuint line_vao, line_vbo; // For rendering connection lines


// Structure to store character glyph data
typedef struct {
    GLuint texture_id; // Texture ID for the glyph
    int width, height; // Glyph dimensions
    int bearing_x, bearing_y; // Offset from baseline to left/top
    unsigned int advance; // Advance to next glyph
} Character;

static SDL_Window *window = NULL;
static SDL_GLContext gl_context = NULL;
static GLuint shader_program, vao, vbo;
static GLuint text_shader_program, text_vao, text_vbo; // For text rendering
static FT_Library ft;
static FT_Face face;
static Character characters[128]; // Store ASCII characters


// Vertex shader for lines
const char *line_vertex_shader_src =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "}\n";

// Fragment shader for lines
const char *line_fragment_shader_src =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White line\n"
    "}\n";

static GLuint line_shader_program;

// Vertex shader for the square
const char *vertex_shader_src =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "}\n";

// Fragment shader for the square
const char *fragment_shader_src =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec3 color;\n"
    "void main() {\n"
    "    FragColor = vec4(color, 1.0);\n"
    "}\n";

// Vertex shader for text (includes texture coordinates)
const char *text_vertex_shader_src =
    "#version 330 core\n"
    "layout(location = 0) in vec4 aPosTex; // <vec2 pos, vec2 tex>\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(aPosTex.xy, 0.0, 1.0);\n"
    "    TexCoord = aPosTex.zw;\n"
    "}\n";

// Fragment shader for text
const char *text_fragment_shader_src =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D textTexture;\n"
    "uniform vec3 textColor;\n"
    "void main() {\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(textTexture, TexCoord).r);\n"
    "    FragColor = vec4(textColor, 1.0) * sampled;\n"
    "}\n";

GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        printf("Shader compilation failed: %s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void init_freetype(void) {
    // Initialize FreeType
    if (FT_Init_FreeType(&ft)) {
        printf("Failed to initialize FreeType\n");
        exit(1);
    }

    // Load font (ensure arial.ttf is in your project directory)
    if (FT_New_Face(ft, "Kenney Mini.ttf", 0, &face)) {
        printf("Failed to load font\n");
        exit(1);
    }

    // Set pixel size for the font
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 ASCII characters
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            printf("Failed to load glyph '%c'\n", c);
            continue;
        }

        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character data
        characters[c] = (Character){
            .texture_id = texture,
            .width = face->glyph->bitmap.width,
            .height = face->glyph->bitmap.rows,
            .bearing_x = face->glyph->bitmap_left,
            .bearing_y = face->glyph->bitmap_top,
            .advance = face->glyph->advance.x
        };
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void init_text_opengl(void) {
    // Compile text shaders
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, text_vertex_shader_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, text_fragment_shader_src);
    if (!vertex_shader || !fragment_shader) {
        exit(1);
    }

    // Link text shader program
    text_shader_program = glCreateProgram();
    glAttachShader(text_shader_program, vertex_shader);
    glAttachShader(text_shader_program, fragment_shader);
    glLinkProgram(text_shader_program);
    GLint success;
    glGetProgramiv(text_shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(text_shader_program, 512, NULL, info_log);
        printf("Text shader program linking failed: %s\n", info_log);
        exit(1);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Set up text VAO and VBO
    glGenVertexArrays(1, &text_vao);
    glGenBuffers(1, &text_vbo);
    glBindVertexArray(text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}



void init_opengl(void) {
    if (!gladLoaderLoadGL()) {
        printf("Failed to initialize GLAD\n");
        exit(1);
    }

    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);
    glViewport(0, 0, win_w, win_h);

    // Compile shaders for nodes
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    if (!vertex_shader || !fragment_shader) {
        exit(1);
    }
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        printf("Shader program linking failed: %s\n", info_log);
        exit(1);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Compile shaders for lines
    vertex_shader = compile_shader(GL_VERTEX_SHADER, line_vertex_shader_src);
    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, line_fragment_shader_src);
    if (!vertex_shader || !fragment_shader) {
        exit(1);
    }
    line_shader_program = glCreateProgram();
    glAttachShader(line_shader_program, vertex_shader);
    glAttachShader(line_shader_program, fragment_shader);
    glLinkProgram(line_shader_program);
    glGetProgramiv(line_shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(line_shader_program, 512, NULL, info_log);
        printf("Line shader program linking failed: %s\n", info_log);
        exit(1);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Initialize two nodes
    for (int i = 0; i < node_count; i++) {
        nodes[i].x = -0.5f + i * 1.0f; // Space nodes apart
        nodes[i].y = 0.0f;
        nodes[i].width = 0.5f;
        nodes[i].height = 0.5f;
        nodes[i].io_width = 0.1f;
        nodes[i].io_height = 0.1f;
        nodes[i].input_x = nodes[i].x - nodes[i].width / 2.0f - nodes[i].io_width / 2.0f;
        nodes[i].input_y = nodes[i].y;
        nodes[i].output_x = nodes[i].x + nodes[i].width / 2.0f + nodes[i].io_width / 2.0f;
        nodes[i].output_y = nodes[i].y;
        nodes[i].connected_to = -1; // No initial connection
        nodes[i].is_input_connected = false; // Input not connected
        nodes[i].is_output_connected = false; // Output not connected
        snprintf(nodes[i].name, sizeof(nodes[i].name), "Node %d", i); // Set node name

        // Setup main square VAO/VBO
        glGenVertexArrays(1, &nodes[i].vao);
        glGenBuffers(1, &nodes[i].vbo);
        glBindVertexArray(nodes[i].vao);
        glBindBuffer(GL_ARRAY_BUFFER, nodes[i].vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 3, NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Setup input rectangle VAO/VBO
        glGenVertexArrays(1, &nodes[i].input_vao);
        glGenBuffers(1, &nodes[i].input_vbo);
        glBindVertexArray(nodes[i].input_vao);
        glBindBuffer(GL_ARRAY_BUFFER, nodes[i].input_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 3, NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Setup output rectangle VAO/VBO
        glGenVertexArrays(1, &nodes[i].output_vao);
        glGenBuffers(1, &nodes[i].output_vbo);
        glBindVertexArray(nodes[i].output_vao);
        glBindBuffer(GL_ARRAY_BUFFER, nodes[i].output_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 3, NULL, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    // Setup line VAO/VBO
    glGenVertexArrays(1, &line_vao);
    glGenBuffers(1, &line_vbo);
    glBindVertexArray(line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Initialize FreeType and text rendering
    init_freetype();
    init_text_opengl();
}




void update_node_vertices(int index) {
    Node2D *node = &nodes[index];
    // Update main square vertices
    float square_vertices[] = {
        node->x - node->width / 2.0f, node->y + node->height / 2.0f, 0.0f,
        node->x + node->width / 2.0f, node->y + node->height / 2.0f, 0.0f,
        node->x + node->width / 2.0f, node->y - node->height / 2.0f, 0.0f,
        node->x - node->width / 2.0f, node->y - node->height / 2.0f, 0.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, node->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(square_vertices), square_vertices);

    // Update input rectangle vertices
    float input_vertices[] = {
        node->input_x - node->io_width / 2.0f, node->input_y + node->io_height / 2.0f, 0.0f,
        node->input_x + node->io_width / 2.0f, node->input_y + node->io_height / 2.0f, 0.0f,
        node->input_x + node->io_width / 2.0f, node->input_y - node->io_height / 2.0f, 0.0f,
        node->input_x - node->io_width / 2.0f, node->input_y - node->io_height / 2.0f, 0.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, node->input_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(input_vertices), input_vertices);

    // Update output rectangle vertices
    float output_vertices[] = {
        node->output_x - node->io_width / 2.0f, node->output_y + node->io_height / 2.0f, 0.0f,
        node->output_x + node->io_width / 2.0f, node->output_y + node->io_height / 2.0f, 0.0f,
        node->output_x + node->io_width / 2.0f, node->output_y - node->io_height / 2.0f, 0.0f,
        node->output_x - node->io_width / 2.0f, node->output_y - node->io_height / 2.0f, 0.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, node->output_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(output_vertices), output_vertices);
}





void update_square_vertices(void) {
    float vertices[] = {
        square_pos_x - 0.25f, square_pos_y + 0.25f, 0.0f, // Top-left
        square_pos_x + 0.25f, square_pos_y + 0.25f, 0.0f, // Top-right
        square_pos_x + 0.25f, square_pos_y - 0.25f, 0.0f, // Bottom-right
        square_pos_x - 0.25f, square_pos_y - 0.25f, 0.0f  // Bottom-left
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}


// Function to render text
void render_text(const char *text, float x, float y, float scale, float color[3]) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up orthographic projection
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);
    float projection[16] = {
        2.0f / win_w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / win_h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    glUseProgram(text_shader_program);
    glUniform3f(glGetUniformLocation(text_shader_program, "textColor"), color[0], color[1], color[2]);
    glUniformMatrix4fv(glGetUniformLocation(text_shader_program, "projection"), 1, GL_FALSE, projection);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(text_vao);

    // Iterate through all characters
    for (const char *c = text; *c; c++) {
        Character ch = characters[(unsigned char)*c];

        float xpos = x + ch.bearing_x * scale;
        float ypos = y - ch.bearing_y * scale; // Align to top
        float w = ch.width * scale;
        float h = ch.height * scale;

        // Update VBO for each character (flipped texture coordinates)
        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 1.0f }, // Top-left
            { xpos,     ypos,     0.0f, 0.0f }, // Bottom-left
            { xpos + w, ypos,     1.0f, 0.0f }, // Bottom-right
            { xpos,     ypos + h, 0.0f, 1.0f }, // Top-left
            { xpos + w, ypos,     1.0f, 0.0f }, // Bottom-right
            { xpos + w, ypos + h, 1.0f, 1.0f }  // Top-right
        };

        // Render glyph texture
        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor
        x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}



int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("SDL3 GLAD Square", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("GL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    init_opengl();


    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                glViewport(0, 0, event.window.data1, event.window.data2);
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int win_w, win_h;
                    SDL_GetWindowSize(window, &win_w, &win_h);
                    float mouse_x = (2.0f * event.button.x / win_w) - 1.0f;
                    float mouse_y = 1.0f - (2.0f * event.button.y / win_h);

                    // Check for dragging (main square only)
                    for (int i = 0; i < node_count; i++) {
                        if (mouse_x >= nodes[i].x - nodes[i].width / 2.0f &&
                            mouse_x <= nodes[i].x + nodes[i].width / 2.0f &&
                            mouse_y >= nodes[i].y - nodes[i].height / 2.0f &&
                            mouse_y <= nodes[i].y + nodes[i].height / 2.0f) {
                            is_dragging = true;
                            dragging_node = i;
                            drag_offset_x = nodes[i].x - mouse_x;
                            drag_offset_y = nodes[i].y - mouse_y;
                            break;
                        }
                    }

                    // Check for connection start, disconnection, or completion
                    if (!is_dragging) {
                        for (int i = 0; i < node_count; i++) {
                            // Check input rectangle
                            if (mouse_x >= nodes[i].input_x - nodes[i].io_width / 2.0f &&
                                mouse_x <= nodes[i].input_x + nodes[i].io_width / 2.0f &&
                                mouse_y >= nodes[i].input_y - nodes[i].io_height / 2.0f &&
                                mouse_y <= nodes[i].input_y + nodes[i].io_height / 2.0f) {
                                if (nodes[i].is_input_connected) {
                                    // Disconnect input
                                    int target_node = nodes[i].connected_to;
                                    if (target_node != -1) {
                                        nodes[target_node].is_output_connected = false;
                                    }
                                    nodes[i].connected_to = -1;
                                    nodes[i].is_input_connected = false;
                                    is_connecting = false;
                                    connecting_node = -1;
                                    is_connecting_from_output = false;
                                } else if (is_connecting && is_connecting_from_output && i != connecting_node &&
                                        !nodes[i].is_input_connected) {
                                    // Complete connection from output to input
                                    nodes[i].connected_to = connecting_node;
                                    nodes[i].is_input_connected = true;
                                    nodes[connecting_node].is_output_connected = true;
                                    is_connecting = false;
                                    connecting_node = -1;
                                    is_connecting_from_output = false;
                                } else if (!is_connecting && !nodes[i].is_input_connected) {
                                    // Start connecting from input
                                    is_connecting = true;
                                    connecting_node = i;
                                    is_connecting_from_output = false;
                                    connecting_x = nodes[i].input_x;
                                    connecting_y = nodes[i].input_y;
                                }
                                break;
                            }
                            // Check output rectangle
                            if (mouse_x >= nodes[i].output_x - nodes[i].io_width / 2.0f &&
                                mouse_x <= nodes[i].output_x + nodes[i].io_width / 2.0f &&
                                mouse_y >= nodes[i].output_y - nodes[i].io_height / 2.0f &&
                                mouse_y <= nodes[i].output_y + nodes[i].io_height / 2.0f) {
                                if (nodes[i].is_output_connected) {
                                    // Disconnect output
                                    for (int j = 0; j < node_count; j++) {
                                        if (nodes[j].connected_to == i) {
                                            nodes[j].connected_to = -1;
                                            nodes[j].is_input_connected = false;
                                            break;
                                        }
                                    }
                                    nodes[i].is_output_connected = false;
                                    is_connecting = false;
                                    connecting_node = -1;
                                    is_connecting_from_output = false;
                                } else if (!is_connecting && !nodes[i].is_output_connected) {
                                    // Start connecting from output
                                    is_connecting = true;
                                    connecting_node = i;
                                    is_connecting_from_output = true;
                                    connecting_x = nodes[i].output_x;
                                    connecting_y = nodes[i].output_y;
                                } else if (is_connecting && !is_connecting_from_output && i != connecting_node &&
                                        !nodes[i].is_output_connected) {
                                    // Complete connection from input to output
                                    nodes[connecting_node].connected_to = i;
                                    nodes[connecting_node].is_input_connected = true;
                                    nodes[i].is_output_connected = true;
                                    is_connecting = false;
                                    connecting_node = -1;
                                    is_connecting_from_output = false;
                                }
                                break;
                            }
                        }
                    }

                    // Cancel connection if clicking elsewhere
                    if (is_connecting && !is_dragging) {
                        bool hit_input_or_output = false;
                        for (int i = 0; i < node_count; i++) {
                            if ((mouse_x >= nodes[i].input_x - nodes[i].io_width / 2.0f &&
                                mouse_x <= nodes[i].input_x + nodes[i].io_width / 2.0f &&
                                mouse_y >= nodes[i].input_y - nodes[i].io_height / 2.0f &&
                                mouse_y <= nodes[i].input_y + nodes[i].io_height / 2.0f) ||
                                (mouse_x >= nodes[i].output_x - nodes[i].io_width / 2.0f &&
                                mouse_x <= nodes[i].output_x + nodes[i].io_width / 2.0f &&
                                mouse_y >= nodes[i].output_y - nodes[i].io_height / 2.0f &&
                                mouse_y <= nodes[i].output_y + nodes[i].io_height / 2.0f)) {
                                hit_input_or_output = true;
                                break;
                            }
                        }
                        if (!hit_input_or_output) {
                            is_connecting = false;
                            connecting_node = -1;
                            is_connecting_from_output = false;
                        }
                    }
                }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    is_dragging = false;
                    dragging_node = -1;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION && is_dragging) {
                int win_w, win_h;
                SDL_GetWindowSize(window, &win_w, &win_h);
                float mouse_x = (2.0f * event.motion.x / win_w) - 1.0f;
                float mouse_y = 1.0f - (2.0f * event.motion.y / win_h);
                nodes[dragging_node].x = mouse_x + drag_offset_x;
                nodes[dragging_node].y = mouse_y + drag_offset_y;
                nodes[dragging_node].input_x = nodes[dragging_node].x - nodes[dragging_node].width / 2.0f - nodes[dragging_node].io_width / 2.0f;
                nodes[dragging_node].input_y = nodes[dragging_node].y;
                nodes[dragging_node].output_x = nodes[dragging_node].x + nodes[dragging_node].width / 2.0f + nodes[dragging_node].io_width / 2.0f;
                nodes[dragging_node].output_y = nodes[dragging_node].y;
                update_node_vertices(dragging_node);
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION && is_connecting) {
                int win_w, win_h;
                SDL_GetWindowSize(window, &win_w, &win_h);
                connecting_x = (2.0f * event.motion.x / win_w) - 1.0f;
                connecting_y = 1.0f - (2.0f * event.motion.y / win_h);
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw all nodes
        glUseProgram(shader_program);
        GLint color_loc = glGetUniformLocation(shader_program, "color");
        for (int i = 0; i < node_count; i++) {
            update_node_vertices(i); // Ensure vertices are up-to-date
            // Draw main square (blue)
            glUniform3f(color_loc, 0.0f, 0.0f, 1.0f);
            glBindVertexArray(nodes[i].vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);

            // Draw input rectangle (green)
            glUniform3f(color_loc, 0.0f, 1.0f, 0.0f);
            glBindVertexArray(nodes[i].input_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);

            // Draw output rectangle (red)
            glUniform3f(color_loc, 1.0f, 0.0f, 0.0f);
            glBindVertexArray(nodes[i].output_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
        }

        // Draw connections
        glUseProgram(line_shader_program);
        glBindVertexArray(line_vao);
        for (int i = 0; i < node_count; i++) {
            if (nodes[i].connected_to != -1) {
                float line_vertices[] = {
                    nodes[i].input_x, nodes[i].input_y, 0.0f,
                    nodes[nodes[i].connected_to].output_x, nodes[nodes[i].connected_to].output_y, 0.0f
                };
                glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_vertices), line_vertices);
                glDrawArrays(GL_LINES, 0, 2);
            }
        }
        if (is_connecting) {
            float line_vertices[] = {
                is_connecting_from_output ? nodes[connecting_node].output_x : nodes[connecting_node].input_x,
                is_connecting_from_output ? nodes[connecting_node].output_y : nodes[connecting_node].input_y,
                0.0f,
                connecting_x, connecting_y, 0.0f
            };
            glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_vertices), line_vertices);
            glDrawArrays(GL_LINES, 0, 2);
        }
        glBindVertexArray(0);

        // Render node names and "Hello World"
        float text_color[3] = {1.0f, 1.0f, 1.0f};
        for (int i = 0; i < node_count; i++) {
            // Convert NDC to pixel coordinates for text positioning
            int win_w, win_h;
            SDL_GetWindowSize(window, &win_w, &win_h);
            float pixel_x = (nodes[i].x - nodes[i].width / 2.0f + 1.0f) * win_w / 2.0f + 10.0f; // Offset slightly inside square
            float pixel_y = (1.0f - (nodes[i].y + nodes[i].height / 2.0f)) * win_h / 2.0f + 10.0f; // Top-left of square
            render_text(nodes[i].name, pixel_x, pixel_y, 0.5f, text_color); // Smaller scale for node names
        }
        render_text("Hello World", 50.0f, 50.0f, 1.0f, text_color);

        SDL_GL_SwapWindow(window);
    }

    // Clean up FreeType resources
    for (int i = 0; i < 128; i++) {
        glDeleteTextures(1, &characters[i].texture_id);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Clean up FreeType resources
    for (int i = 0; i < 128; i++) {
        glDeleteTextures(1, &characters[i].texture_id);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Clean up FreeType resources
    for (int i = 0; i < 128; i++) {
        glDeleteTextures(1, &characters[i].texture_id);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Clean up OpenGL resources
    for (int i = 0; i < node_count; i++) {
        glDeleteVertexArrays(1, &nodes[i].vao);
        glDeleteBuffers(1, &nodes[i].vbo);
        glDeleteVertexArrays(1, &nodes[i].input_vao);
        glDeleteBuffers(1, &nodes[i].input_vbo);
        glDeleteVertexArrays(1, &nodes[i].output_vao);
        glDeleteBuffers(1, &nodes[i].output_vbo);
    }
    glDeleteVertexArrays(1, &line_vao);
    glDeleteBuffers(1, &line_vbo);
    glDeleteProgram(shader_program);
    glDeleteProgram(line_shader_program);
    glDeleteVertexArrays(1, &text_vao);
    glDeleteBuffers(1, &text_vbo);
    glDeleteProgram(text_shader_program);
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}