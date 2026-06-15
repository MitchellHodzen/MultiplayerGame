#include "window_state.h"
#include <SDL3/SDL.H>
#include <SDL3_ttf/SDL_ttf.h>
#include <clay.h>

bool InitializeSDL(SDL_Window** window, SDL_Renderer** renderer, TTF_TextEngine** textEngine, int screen_width, int screen_height)
{
    if (!SDL_SetAppMetadata("mygame", "1.0", "com.mygame"))
    {
        SDL_Log("Couldn't Set SDL Metadata: %s", SDL_GetError());
        return false;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialize SDL_ttf: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("gaem", screen_width, screen_height, SDL_WINDOW_RESIZABLE, window, renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    *textEngine = TTF_CreateRendererTextEngine(*renderer);
    if (*textEngine == NULL) {
        SDL_Log("Failed to create text engine from renderer: %s", SDL_GetError());
        // Clean up SDL
        SDL_DestroyRenderer(*renderer);
        *renderer = NULL;
        SDL_DestroyWindow(*window);
        *window = NULL;
        SDL_Quit();
        return false;
    }

    return true;
}

bool LoadFont(TTF_Font** font, const char* path)
{
    *font = TTF_OpenFont(path, 24);
    if (font == NULL) {
        SDL_Log("Failed to load font");
        return false;
    }

    return true;
}

bool InitializeClay(void** clayArena, TTF_Font* font, int screenWidth, int screenHeight, Clay_ErrorHandler errorHandler, Clay_Dimensions (*measureTextFunction)(Clay_StringSlice text, Clay_TextElementConfig *config))
{
    uint64_t totalMemorySize = Clay_MinMemorySize();
    *clayArena = malloc(totalMemorySize);
    if ((*clayArena) == NULL)
    {
        SDL_Log("Could not allocate memory for Clay arena");
        return false;
    }

    Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, *clayArena), (Clay_Dimensions) { screenWidth, screenHeight }, errorHandler);
    Clay_SetMeasureTextFunction(measureTextFunction, font);
    return true;
}

bool Window_State_Init(struct Window_State** window_state, unsigned int screen_width, unsigned int screen_height, Clay_ErrorHandler errorHandler, Clay_Dimensions (*measureTextFunction)(Clay_StringSlice text, Clay_TextElementConfig *config))
{
    *window_state = (struct Window_State*) malloc(sizeof(struct Window_State));
    if (*window_state == NULL)
    {
        SDL_Log("Could not allocate window state struct");
        return false;
    }

    if (InitializeSDL(&(*window_state)->window, &(*window_state)->renderer, &(*window_state)->textEngine, screen_width, screen_height))
    {
        SDL_Log("SDL Initialized");
    }
    else
    {
        SDL_Log("SDL Initialization Failed");
        return false;
    }

    if (LoadFont(&(*window_state)->font, "resources/fonts/PixelGamingRegular-d9w0g.ttf"))
    {
        SDL_Log("Font Loaded");
    }
    else
    {
        SDL_Log("Failed to load font");
        return false;
    }

    if (InitializeClay(&(*window_state)->clayArena, (*window_state)->font, screen_width, screen_height, errorHandler, measureTextFunction))
    {
        SDL_Log("Clay Initialized");
    }
    else
    {
        SDL_Log("Clay Initialization Failed");
        return false;
    }

    return true;
}

void Window_State_Free(struct Window_State** window_state)
{
    SDL_Log("Free clay arena");
    free((*window_state)->clayArena);
    (*window_state)->clayArena = NULL;

    SDL_Log("Free font");
    TTF_CloseFont((*window_state)->font);
    (*window_state)->font = NULL;
    SDL_Log("Destroy text engine");
    TTF_DestroyRendererTextEngine((*window_state)->textEngine);
    (*window_state)->textEngine = NULL;
    SDL_Log("Destroy renderer");
    SDL_DestroyRenderer((*window_state)->renderer);
    (*window_state)->renderer = NULL;
    SDL_Log("Destroy window");
    SDL_DestroyWindow((*window_state)->window);
    (*window_state)->window = NULL;
    SDL_Log("Sdl quit");
    SDL_Quit();
    *window_state = NULL;
}