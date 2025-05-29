#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

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
    if (!SDL_CreateWindowAndRenderer("Font Test", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Window/Renderer creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    printf("TTF_OpenFont\n");
    TTF_Font *font = TTF_OpenFont("Kenney Mini.ttf", 16);
    if (!font) {
        SDL_Log("Font loading failed: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Render text
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color black = {0, 0, 0, 255};
    const char *text = "Hello, Terminal!";
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, strlen(text), white);
    if (!surface) {
        SDL_Log("Text rendering failed: %s", SDL_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Define square mesh (four vertices for a 100x100 pixel square)
    SDL_Vertex vertices[4] = {
        {{100.0f, 100.0f}, {255, 0, 0, 255}, {0.0f, 0.0f}}, // Top-left, red
        {{200.0f, 100.0f}, {0, 255, 0, 255}, {1.0f, 0.0f}}, // Top-right, green
        {{200.0f, 200.0f}, {0, 0, 255, 255}, {1.0f, 1.0f}}, // Bottom-right, blue
        {{100.0f, 200.0f}, {255, 255, 0, 255}, {0.0f, 1.0f}} // Bottom-left, yellow
    };
    Uint32 indices[6] = {0, 1, 2, 0, 2, 3}; // Two triangles to form the square

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Render text
        SDL_FRect dest = {10.0f, 10.0f, 0.0f, 0.0f};
        SDL_GetTextureSize(texture, &dest.w, &dest.h);
        SDL_RenderTexture(renderer, texture, NULL, &dest);

        // Render square mesh
        SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}