#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_NODES 10
#define MAX_PORTS 20 // Cap on inputs and outputs per node
#define MAX_CONNECTIONS 50 // Max connections to prevent overflow
#define GRID_SIZE 20.0f // NEW: Grid size for snapping

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *font = NULL;

/* Node structure */
typedef struct {
    float x, y; // Top-left position in world coordinates
    SDL_Vertex vertices[4]; // Square mesh vertices in world coordinates
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

/* Connection structure */
typedef struct {
    int output_node_idx; // Index of node with output port
    int output_port_idx; // Index of output port
    int input_node_idx;  // Index of node with input port
    int input_port_idx;  // Index of input port
} Connection;

/* Global state */
static Node nodes[MAX_NODES];
static int num_nodes = 0;
static Uint32 indices[6] = {0, 1, 2, 0, 2, 3};
static Connection connections[MAX_CONNECTIONS];
static int num_connections = 0;
static bool is_connecting = false;
static int start_node_idx = -1;
static int start_port_idx = -1;
static float connect_start_x, connect_start_y; // Start of active connection line
static float view_x = 0.0f, view_y = 0.0f; // View offset for panning
static float zoom = 1.0f; // Zoom scale
static bool is_panning = false;
static float pan_start_x, pan_start_y;

/* Initialize a node */
void init_node(Node *node, float x, float y, const char *name, int num_inputs, int num_outputs) {
    // NEW: Snap position to grid
    node->x = roundf(x / GRID_SIZE) * GRID_SIZE;
    node->y = roundf(y / GRID_SIZE) * GRID_SIZE;
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->num_inputs = SDL_min(num_inputs, MAX_PORTS);
    node->num_outputs = SDL_min(num_outputs, MAX_PORTS);
    node->is_dragging = false;
    node->drag_offset_x = 0.0f;
    node->drag_offset_y = 0.0f;

    // Initialize vertices (100x100 square in world coordinates)
    SDL_Color blue = {0, 0, 255, 255};
    float r = blue.r / 255.0f;
    float g = blue.g / 255.0f;
    float b = blue.b / 255.0f;
    float a = blue.a / 255.0f;
    node->vertices[0] = (SDL_Vertex){{node->x, node->y}, {r, g, b, a}, {0.0f, 0.0f}};           // Top-left
    node->vertices[1] = (SDL_Vertex){{node->x + 100.0f, node->y}, {r, g, b, a}, {1.0f, 0.0f}}; // Top-right
    node->vertices[2] = (SDL_Vertex){{node->x + 100.0f, node->y + 100.0f}, {r, g, b, a}, {1.0f, 1.0f}}; // Bottom-right
    node->vertices[3] = (SDL_Vertex){{node->x, node->y + 100.0f}, {r, g, b, a}, {0.0f, 1.0f}}; // Bottom-left
}

/* Check if a point is inside a node (adjusted for view and zoom) */
bool is_point_in_node(const Node *node, float mx, float my) {
    float x_min = (node->vertices[0].position.x + view_x) * zoom;
    float x_max = (node->vertices[2].position.x + view_x) * zoom;
    float y_min = (node->vertices[0].position.y + view_y) * zoom;
    float y_max = (node->vertices[2].position.y + view_y) * zoom;
    return mx >= x_min && mx <= x_max && my >= y_min && my <= y_max;
}

/* Check if a point is inside an output port square */
bool is_point_in_output_port(const Node *node, int port_idx, float mx, float my) {
    float x = (node->x + 110.0f + view_x) * zoom;
    float y = (node->y + 30.0f + port_idx * 20.0f + view_y) * zoom;
    return mx >= x && mx <= x + 10.0f * zoom && my >= y && my <= y + 10.0f * zoom;
}

/* Check if a point is inside an input port square */
bool is_point_in_input_port(const Node *node, int port_idx, float mx, float my) {
    float x = (node->x - 20.0f + view_x) * zoom;
    float y = (node->y + 30.0f + port_idx * 20.0f + view_y) * zoom;
    return mx >= x && mx <= x + 10.0f * zoom && my >= y && my <= y + 10.0f * zoom;
}

/* NEW: Check if a connection to an input port already exists */
bool has_existing_connection_to_input(int input_node_idx, int input_port_idx) {
    for (int i = 0; i < num_connections; i++) {
        if (connections[i].input_node_idx == input_node_idx &&
            connections[i].input_port_idx == input_port_idx) {
            return true;
        }
    }
    return false;
}

/* Update node position during drag */
void update_node_position(Node *node, float mx, float my) {
    // Adjust mouse position for view and zoom
    node->x = (mx / zoom - view_x) - node->drag_offset_x;
    node->y = (my / zoom - view_y) - node->drag_offset_y;
    // NEW: Snap to grid
    node->x = roundf(node->x / GRID_SIZE) * GRID_SIZE;
    node->y = roundf(node->y / GRID_SIZE) * GRID_SIZE;
    // Update vertices
    node->vertices[0].position.x = node->x;
    node->vertices[0].position.y = node->y;
    node->vertices[1].position.x = node->x + 100.0f;
    node->vertices[1].position.y = node->y;
    node->vertices[2].position.x = node->x + 100.0f;
    node->vertices[2].position.y = node->y + 100.0f;
    node->vertices[3].position.x = node->x;
    node->vertices[3].position.y = node->y + 100.0f;
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
        snprintf(node->inputs[i], 16, "In%d", i + 1);
        surface = TTF_RenderText_Solid(font, node->inputs[i], strlen(node->inputs[i]), white);
        if (surface) {
            node->input_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
        }
    }
    for (int i = 0; i < node->num_outputs; i++) {
        snprintf(node->outputs[i], 16, "Out%d", i + 1);
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

/* NEW: Delete a node and its connections */
void delete_node(int node_idx) {
    if (node_idx < 0 || node_idx >= num_nodes) return;
    // Remove associated connections
    int i = 0;
    while (i < num_connections) {
        if (connections[i].input_node_idx == node_idx || connections[i].output_node_idx == node_idx) {
            connections[i] = connections[--num_connections];
        } else {
            // Adjust connection indices if nodes after node_idx shift
            if (connections[i].input_node_idx > node_idx) connections[i].input_node_idx--;
            if (connections[i].output_node_idx > node_idx) connections[i].output_node_idx--;
            i++;
        }
    }
    // Clean up node resources
    destroy_node(&nodes[node_idx]);
    // Shift remaining nodes
    for (int j = node_idx; j < num_nodes - 1; j++) {
        nodes[j] = nodes[j + 1];
    }
    num_nodes--;
    printf("Deleted Node %d\n", node_idx + 1);
}

/* Draw debug rectangle for node */
void draw_debug_rect(const Node *node) {
    SDL_FRect rect = {(node->x + view_x) * zoom, (node->y + view_y) * zoom, 100.0f * zoom, 100.0f * zoom};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red outline
    SDL_RenderRect(renderer, &rect);
}

/* Draw colored squares for input/output ports */
void draw_port_squares(const Node *node) {
    // Input ports (green squares)
    for (int j = 0; j < node->num_inputs; j++) {
        SDL_FRect input_rect = {
            (node->x - 20.0f + view_x) * zoom,
            (node->y + 30.0f + j * 20.0f + view_y) * zoom,
            10.0f * zoom,
            10.0f * zoom
        };
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        SDL_RenderFillRect(renderer, &input_rect);
    }
    // Output ports (red squares)
    for (int j = 0; j < node->num_outputs; j++) {
        SDL_FRect output_rect = {
            (node->x + 110.0f + view_x) * zoom,
            (node->y + 30.0f + j * 20.0f + view_y) * zoom,
            10.0f * zoom,
            10.0f * zoom
        };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
        SDL_RenderFillRect(renderer, &output_rect);
    }
}

/* Draw connections */
void draw_connections(void) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White lines
    for (int i = 0; i < num_connections; i++) {
        Connection *conn = &connections[i];
        Node *output_node = &nodes[conn->output_node_idx];
        Node *input_node = &nodes[conn->input_node_idx];
        float x1 = (output_node->x + 115.0f + view_x) * zoom; // Center of output port
        float y1 = (output_node->y + 35.0f + conn->output_port_idx * 20.0f + view_y) * zoom;
        float x2 = (input_node->x - 15.0f + view_x) * zoom; // Center of input port
        float y2 = (input_node->y + 35.0f + conn->input_port_idx * 20.0f + view_y) * zoom;
        SDL_RenderLine(renderer, x1, y1, x2, y2);
    }
}

int main(int argc, char *argv[]) {
    printf("SDL3 freetype\n");
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
    if (!SDL_CreateWindowAndRenderer("Node2D Editor Test", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        printf("Window/Renderer failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    const char *renderer_name = SDL_GetRendererName(renderer);
    if (renderer_name) {
        printf("Current Renderer: %s\n", renderer_name);
    } else {
        printf("Failed to get renderer name: %s\n", SDL_GetError());
    }

    printf("TTF_OpenFont\n");
    font = TTF_OpenFont("Kenney Mini.ttf", 16);
    if (!font) {
        printf("Font load failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize test nodes
    num_nodes = 2;
    init_node(&nodes[0], 280.0f, 200.0f, "Node 1", 2, 1); // 2 inputs, 1 output
    init_node(&nodes[1], 300.0f, 100.0f, "Node 2", 1, 2); // 1 input, 2 outputs
    for (int i = 0; i < num_nodes; i++) {
        create_node_textures(&nodes[i]);
    }

    // Main loop
    bool running = true;
    Node *dragged_node = NULL;
    float mouse_x = 0.0f, mouse_y = 0.0f;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float mx = (float)event.button.x;
                    float my = (float)event.button.y;
                    if (!is_connecting) {
                        for (int i = 0; i < num_nodes; i++) {
                            for (int j = 0; j < nodes[i].num_outputs; j++) {
                                if (is_point_in_output_port(&nodes[i], j, mx, my)) {
                                    is_connecting = true;
                                    start_node_idx = i;
                                    start_port_idx = j;
                                    connect_start_x = (nodes[i].x + 115.0f + view_x) * zoom;
                                    connect_start_y = (nodes[i].y + 35.0f + j * 20.0f + view_y) * zoom;
                                    printf("Start connection from Node %d, Output %d\n", i, j);
                                    break;
                                }
                            }
                            if (is_connecting) break;
                        }
                    }
                    if (is_connecting) {
                        for (int i = 0; i < num_nodes; i++) {
                            for (int j = 0; j < nodes[i].num_inputs; j++) {
                                if (is_point_in_input_port(&nodes[i], j, mx, my)) {
                                    // NEW: Validate connection
                                    if (start_node_idx != i && // Prevent self-connection
                                        !has_existing_connection_to_input(i, j)) { // Prevent multiple connections to input
                                        if (num_connections < MAX_CONNECTIONS) {
                                            connections[num_connections++] = (Connection){
                                                start_node_idx, start_port_idx, i, j
                                            };
                                            printf("Connected Node %d, Output %d to Node %d, Input %d\n",
                                                start_node_idx, start_port_idx, i, j);
                                        }
                                    } else {
                                        printf("Invalid connection: Self-connection or input already connected\n");
                                    }
                                    is_connecting = false;
                                    start_node_idx = -1;
                                    start_port_idx = -1;
                                    break;
                                }
                            }
                            if (!is_connecting) break;
                        }
                    }
                    if (!is_connecting && !dragged_node) {
                        for (int i = num_nodes - 1; i >= 0; i--) {
                            if (is_point_in_node(&nodes[i], mx, my)) {
                                dragged_node = &nodes[i];
                                dragged_node->is_dragging = true;
                                dragged_node->drag_offset_x = (mx / zoom - view_x) - nodes[i].x;
                                dragged_node->drag_offset_y = (my / zoom - view_y) - nodes[i].y;
                                printf("Dragging %s at (%f, %f), offset (%f, %f)\n",
                                        nodes[i].name, mx, my,
                                        dragged_node->drag_offset_x, dragged_node->drag_offset_y);
                                break;
                            }
                        }
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    float mx = (float)event.button.x;
                    float my = (float)event.button.y;
                    bool near_input = false;
                    for (int i = 0; i < num_nodes; i++) {
                        for (int j = 0; j < nodes[i].num_inputs; j++) {
                            if (is_point_in_input_port(&nodes[i], j, mx, my)) {
                                near_input = true;
                                for (int k = 0; k < num_connections; k++) {
                                    if (connections[k].input_node_idx == i &&
                                        connections[k].input_port_idx == j) {
                                        connections[k] = connections[--num_connections];
                                        printf("Disconnected input %d from Node %d\n", j, i);
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        if (near_input) break;
                    }
                    // NEW: Delete node if clicked on node body
                    if (!near_input) {
                        for (int i = num_nodes - 1; i >= 0; i--) {
                            if (is_point_in_node(&nodes[i], mx, my)) {
                                delete_node(i);
                                break;
                            }
                        }
                    }
                    // NEW: Create new node only if not deleting
                    if (!near_input && num_nodes < MAX_NODES) {
                        char name[32];
                        snprintf(name, sizeof(name), "Node %d", num_nodes + 1);
                        init_node(&nodes[num_nodes], mx / zoom - view_x, my / zoom - view_y, name, 1, 1);
                        create_node_textures(&nodes[num_nodes]);
                        printf("Added %s at (%f, %f)\n", name, mx / zoom - view_x, my / zoom - view_y);
                        num_nodes++;
                    }
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    is_panning = true;
                    pan_start_x = (float)event.button.x;
                    pan_start_y = (float)event.button.y;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (dragged_node) {
                        printf("Finished dragging %s\n", dragged_node->name);
                        dragged_node->is_dragging = false;
                        dragged_node = NULL;
                    }
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    is_panning = false;
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mouse_x = (float)event.motion.x;
                mouse_y = (float)event.motion.y;
                if (dragged_node) {
                    update_node_position(dragged_node, mouse_x, mouse_y);
                    printf("Dragging %s to (%f, %f), new pos (%f, %f)\n",
                           dragged_node->name, mouse_x, mouse_y,
                           dragged_node->x, dragged_node->y);
                }
                if (is_panning) {
                    view_x += (mouse_x - pan_start_x) / zoom;
                    view_y += (mouse_y - pan_start_y) / zoom;
                    pan_start_x = mouse_x;
                    pan_start_y = mouse_y;
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (event.wheel.y != 0) {
                    float old_zoom = zoom;
                    zoom += event.wheel.y * 0.1f;
                    zoom = SDL_clamp(zoom, 0.5f, 2.0f); // Min 0.5, max 2.0
                    view_x = mouse_x / zoom - (mouse_x / old_zoom - view_x);
                    view_y = mouse_y / zoom - (mouse_y / old_zoom - view_y);
                    printf("Zoom: %f\n", zoom);
                }
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw connections
        draw_connections();

        // Draw active connection line
        if (is_connecting) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderLine(renderer, connect_start_x, connect_start_y, mouse_x, mouse_y);
        }

        // Render nodes
        for (int i = 0; i < num_nodes; i++) {
            // Transform vertices for rendering
            SDL_Vertex transformed[4];
            for (int j = 0; j < 4; j++) {
                transformed[j] = nodes[i].vertices[j];
                transformed[j].position.x = (transformed[j].position.x + view_x) * zoom;
                transformed[j].position.y = (transformed[j].position.y + view_y) * zoom;
            }
            SDL_RenderGeometry(renderer, NULL, transformed, 4, indices, 6);

            if (nodes[i].is_dragging) {
                draw_debug_rect(&nodes[i]);
            }
            SDL_FRect name_dest = {
                (nodes[i].x + 10.0f + view_x) * zoom,
                (nodes[i].y + 10.0f + view_y) * zoom,
                0.0f, 0.0f
            };
            SDL_GetTextureSize(nodes[i].name_texture, &name_dest.w, &name_dest.h);
            name_dest.w *= zoom;
            name_dest.h *= zoom;
            SDL_RenderTexture(renderer, nodes[i].name_texture, NULL, &name_dest);
            for (int j = 0; j < nodes[i].num_inputs; j++) {
                SDL_FRect input_dest = {
                    (nodes[i].x - 10.0f + view_x) * zoom,
                    (nodes[i].y + 30.0f + j * 20.0f + view_y) * zoom,
                    0.0f, 0.0f
                };
                SDL_GetTextureSize(nodes[i].input_textures[j], &input_dest.w, &input_dest.h);
                input_dest.w *= zoom;
                input_dest.h *= zoom;
                SDL_RenderTexture(renderer, nodes[i].input_textures[j], NULL, &input_dest);
            }
            for (int j = 0; j < nodes[i].num_outputs; j++) {
                SDL_FRect output_dest = {
                    (nodes[i].x + 90.0f + view_x) * zoom,
                    (nodes[i].y + 30.0f + j * 20.0f + view_y) * zoom,
                    0.0f, 0.0f
                };
                SDL_GetTextureSize(nodes[i].output_textures[j], &output_dest.w, &output_dest.h);
                output_dest.w *= zoom;
                output_dest.h *= zoom;
                SDL_RenderTexture(renderer, nodes[i].output_textures[j], NULL, &output_dest);
            }
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