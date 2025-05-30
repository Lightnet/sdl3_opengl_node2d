#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NODES 10
#define MAX_PORTS 20 // Cap on inputs and outputs per node

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *font = NULL;

/* Node structure */
typedef struct {
    float x, y; // Top-left position
    SDL_Vertex vertices[4]; // Square mesh vertices
    char name[32]; // Node name
    char inputs[MAX_PORTS][16]; // Input port names
    char outputs[MAX_PORTS][16]; // Output port names
    int num_inputs, num_outputs; // Number of ports
    SDL_Texture *name_texture; // Texture for node name
    SDL_Texture *input_textures[MAX_PORTS]; // Textures for input labels
    SDL_Texture *output_textures[MAX_PORTS]; // Textures for output labels
    bool is_dragging;
    float drag_offset_x, drag_offset_y;
} Node;

/* Global node array */
static Node nodes[MAX_NODES];
static int num_nodes = 0;
static Uint32 indices[6] = {0, 1, 2, 0, 2, 3};

/* Initialize a node */
void init_node(Node *node, float x, float y, const char *name, int num_inputs, int num_outputs) {
    node->x = x;
    node->y = y;
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->num_inputs = SDL_min(num_inputs, MAX_PORTS);
    node->num_outputs = SDL_min(num_outputs, MAX_PORTS);
    node->is_dragging = false;
    node->drag_offset_x = 0.0f;
    node->drag_offset_y = 0.0f;

    // Initialize vertices (100x100 square)
    SDL_Color blue = {0, 0, 255, 255};
    float r = blue.r / 255.0f;
    float g = blue.g / 255.0f;
    float b = blue.b / 255.0f;
    float a = blue.a / 255.0f;
    node->vertices[0] = (SDL_Vertex){{x, y}, {r, g, b, a}, {0.0f, 0.0f}};           // Top-left
    node->vertices[1] = (SDL_Vertex){{x + 100.0f, y}, {r, g, b, a}, {1.0f, 0.0f}}; // Top-right
    node->vertices[2] = (SDL_Vertex){{x + 100.0f, y + 100.0f}, {r, g, b, a}, {1.0f, 1.0f}}; // Bottom-right
    node->vertices[3] = (SDL_Vertex){{x, y + 100.0f}, {r, g, b, a}, {0.0f, 1.0f}}; // Bottom-left
}

/* Check if a point is inside a node */
bool is_point_in_node(const Node *node, float mx, float my) {
    float x_min = node->vertices[0].position.x;
    float x_max = node->vertices[2].position.x;
    float y_min = node->vertices[0].position.y;
    float y_max = node->vertices[2].position.y;
    return mx >= x_min && mx <= x_max && my >= y_min && my <= y_max;
}

/* Update node position during drag */
void update_node_position(Node *node, float mx, float my) {
    node->x = mx - node->drag_offset_x;
    node->y = my - node->drag_offset_y;
    init_node(node, node->x, node->y, node->name, node->num_inputs, node->num_outputs);
}

/* Render text textures for a node */
void create_node_textures(Node *node) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, node->name, strlen(node->name), white);
    if (surface) {
        node->name_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    for (int i = 0; i < node->num_inputs; i++) {
        surface = TTF_RenderText_Solid(font, node->inputs[i], strlen(node->inputs[i]), white);
        if (surface) {
            node->input_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
        }
    }
    for (int i = 0; i < node->num_outputs; i++) {
        surface = TTF_RenderText_Solid(font, node->outputs[i], strlen(node->outputs[i]), white);
        if (surface) {
            node->output_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
        }
    }
}

/* Clean up node textures */
void destroy_node(Node *node) {
    if (node->name_texture) SDL_DestroyTexture(node->name_texture);
    for (int i = 0; i < node->num_inputs; i++) {
        if (node->input_textures[i]) SDL_DestroyTexture(node->input_textures[i]);
    }
    for (int i = 0; i < node->num_outputs; i++) {
        if (node->output_textures[i]) SDL_DestroyTexture(node->output_textures[i]);
    }
}

/* Draw debug rectangle for node hitbox */
void draw_debug_rect(const Node *node) {
    SDL_FRect rect = {node->x, node->y, 100.0f, 100.0f};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red outline
    SDL_RenderRect(renderer, &rect);
}

/* Draw colored squares for input/output ports */
void draw_port_squares(const Node *node) {
    // Input ports (green squares)
    for (int j = 0; j < node->num_inputs; j++) {
        SDL_FRect input_rect = {node->x - 20.0f, node->y + 30.0f + j * 20.0f, 10.0f, 10.0f};
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        SDL_RenderFillRect(renderer, &input_rect);
    }
    // Output ports (red squares)
    for (int j = 0; j < node->num_outputs; j++) {
        SDL_FRect output_rect = {node->x + 110.0f, node->y + 30.0f + j * 20.0f, 10.0f, 10.0f};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
        SDL_RenderFillRect(renderer, &output_rect);
    }
}

int main(int argc, char *argv[]) {
    printf("SDL3 freetype\n");
    // Initialize SDL and SDL_ttf
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }
    printf("TTF_Init\n");
    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    printf("SDL_CreateWindowAndRenderer\n");
    if (!SDL_CreateWindowAndRenderer("Node Editor Test", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Set blend mode for proper rendering
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Load font
    printf("TTF_OpenFont\n");
    font = TTF_OpenFont("Kenney Mini.ttf", 16);
    if (!font) {
        SDL_Log("Font loading failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize test nodes
    num_nodes = 2;
    init_node(&nodes[0], 100.0f, 100.0f, "Node 1", 2, 1); // 2 inputs, 1 output
    init_node(&nodes[1], 300.0f, 100.0f, "Node 2", 1, 2); // 1 input, 2 outputs
    for (int i = 0; i < num_nodes; i++) {
        create_node_textures(&nodes[i]);
    }

    // Main loop
    bool running = true;
    Node *dragged_node = NULL; // Track the currently dragged node
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        float mx = event.button.x;
                        float my = event.button.y;
                        dragged_node = NULL; // Reset dragged node
                        for (int i = num_nodes - 1; i >= 0; i--) { // Check topmost node first
                            if (is_point_in_node(&nodes[i], mx, my)) {
                                dragged_node = &nodes[i];
                                dragged_node->is_dragging = true;
                                // Calculate offset relative to click position within node
                                dragged_node->drag_offset_x = mx - nodes[i].x;
                                dragged_node->drag_offset_y = my - nodes[i].y;
                                // Log for debugging
                                printf("Start dragging %s at offset (%f, %f)\n", nodes[i].name, 
                                       dragged_node->drag_offset_x, dragged_node->drag_offset_y);
                                break; // Only drag one node
                            }
                        }
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (dragged_node) {
                            printf("Stop dragging %s\n", dragged_node->name);
                        }
                        for (int i = 0; i < num_nodes; i++) {
                            nodes[i].is_dragging = false;
                        }
                        dragged_node = NULL; // Clear dragged node
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (dragged_node) {
                        update_node_position(dragged_node, event.motion.x, event.motion.y);
                        // Log for debugging
                        printf("Dragging %s to (%f, %f)\n", dragged_node->name, 
                               event.motion.x, event.motion.y);
                    }
                    break;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Render nodes
        for (int i = 0; i < num_nodes; i++) {
            // Draw node square
            SDL_RenderGeometry(renderer, NULL, nodes[i].vertices, 4, indices, 6);
            // Draw debug rectangle if dragging
            if (nodes[i].is_dragging) {
                draw_debug_rect(&nodes[i]);
            }
            // Draw node name
            SDL_FRect name_dest = {nodes[i].x + 10.0f, nodes[i].y + 10.0f, 0.0f, 0.0f};
            SDL_GetTextureSize(nodes[i].name_texture, &name_dest.w, &name_dest.h);
            SDL_RenderTexture(renderer, nodes[i].name_texture, NULL, &name_dest);
            // Draw input ports and squares
            for (int j = 0; j < nodes[i].num_inputs; j++) {
                SDL_FRect input_dest = {nodes[i].x - 10.0f, nodes[i].y + 30.0f + j * 20.0f, 0.0f, 0.0f};
                SDL_GetTextureSize(nodes[i].input_textures[j], &input_dest.w, &input_dest.h);
                SDL_RenderTexture(renderer, nodes[i].input_textures[j], NULL, &input_dest);
            }
            // Draw output ports and squares
            for (int j = 0; j < nodes[i].num_outputs; j++) {
                SDL_FRect output_dest = {nodes[i].x + 90.0f, nodes[i].y + 30.0f + j * 20.0f, 0.0f, 0.0f};
                SDL_GetTextureSize(nodes[i].output_textures[j], &output_dest.w, &output_dest.h);
                SDL_RenderTexture(renderer, nodes[i].output_textures[j], NULL, &output_dest);
            }
            // Draw port squares
            draw_port_squares(&nodes[i]);
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    for (int i = 0; i < num_nodes; i++) {
        destroy_node(&nodes[i]);
    }
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}