#include "initialization.h"
#include "component_handles.h"
#include <SDL3/SDL.H>
#include <SDL3_ttf/SDL_ttf.h>
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_transform.h"
#include <clay.h>

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount)
{
    if (!ECDB_Init(ecdb, entityCount, 3))
    {
        SDL_Log("Couldn't initialize component DB");
        return false;
    }

    struct C_Transform defaultTransform = {.parent_id = (*ecdb)->invalidEntityId};
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Transform), &(componentHandles->transforms_handle), &defaultTransform))
    {
        SDL_Log("Couldn't initialize transform component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(SDL_FColor), &(componentHandles->colors_handle), NULL))
    {
        SDL_Log("Couldn't initialize colors component");
        ECDB_Free(ecdb);
        return false;
    }
    
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Input), &(componentHandles->inputs_handle), NULL))
    {
        SDL_Log("Couldn't initialize input component");
        ECDB_Free(ecdb);
        return false;
    }
    return true;
}

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
    SDL_Log("clay alloc");
    uint64_t totalMemorySize = Clay_MinMemorySize();
    SDL_Log("clay alloc mem size: %i", totalMemorySize);
    *clayArena = malloc(totalMemorySize);
    SDL_Log("malloc done");
    if ((*clayArena) == NULL)
    {
        SDL_Log("Could not allocate memory for Clay arena");
        return false;
    }

    SDL_Log("clay init");
    Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, *clayArena), (Clay_Dimensions) { screenWidth, screenHeight }, errorHandler);
    SDL_Log("clay measure text set");
    Clay_SetMeasureTextFunction(measureTextFunction, font);
    return true;
}

bool Game_Data_Init(struct Game_Data** gameData, int screenWidth, int screenHeight, int entityCount, Clay_ErrorHandler errorHandler, Clay_Dimensions (*measureTextFunction)(Clay_StringSlice text, Clay_TextElementConfig *config))
{
    *gameData = (struct Game_Data*) malloc(sizeof(struct Game_Data));
    if (*gameData == NULL)
    {
        SDL_Log("Could not allocate game data struct");
        return false;
    }

    if (InitializeSDL(&(*gameData)->window, &(*gameData)->renderer, &(*gameData)->textEngine, screenWidth, screenHeight))
    {
        SDL_Log("SDL Initialized");
    }
    else
    {
        SDL_Log("SDL Initialization Failed");
        return false;
    }

    if (LoadFont(&(*gameData)->font, "resources/fonts/PixelGamingRegular-d9w0g.ttf"))
    {
        SDL_Log("Font Loaded");
    }
    else
    {
        SDL_Log("Failed to load font");
        return false;
    }

    if (InitializeClay(&(*gameData)->clayArena, (*gameData)->font, screenWidth, screenHeight, errorHandler, measureTextFunction))
    {
        SDL_Log("Clay Initialized");
    }
    else
    {
        SDL_Log("Clay Initialization Failed");
        return false;
    }

    if (InitializeECDB(&(*gameData)->ec, &(*gameData)->componentHandles, entityCount))
    {
        SDL_Log("ECDB Initialized");
    }
    else
    {
        SDL_Log("ECDB Initialization Failed");
        return false;
    }

    if (enet_initialize() == 0)
    {
        SDL_Log("ENet Initialized");
    }
    else
    {
        SDL_Log("ENet Initialization Failed");
        return false;
    }

    if (Net_Initialize(&(*gameData)->netManager, (*gameData)->ec))
    {
        SDL_Log("NetManager Initialized");
    }
    else
    {
        SDL_Log("NetManager Initialization Failed");
        return false;
    }

    return true;
}

void Game_Data_Free(struct Game_Data** gameData)
{
    // Close up
    SDL_Log("free netmgr");
    Net_Free(&(*gameData)->netManager);
    (*gameData)->netManager = NULL;
    SDL_Log("enet deinit");
    enet_deinitialize();

    SDL_Log("free ecdb");
    ECDB_Free(&(*gameData)->ec);
    (*gameData)->ec = NULL;

    SDL_Log("Free clay arena");
    free((*gameData)->clayArena);
    (*gameData)->clayArena = NULL;

    SDL_Log("freeing font");
    TTF_CloseFont((*gameData)->font);
    (*gameData)->font = NULL;
    SDL_Log("destroy text engine");
    TTF_DestroyRendererTextEngine((*gameData)->textEngine);
    (*gameData)->textEngine = NULL;
    SDL_Log("destroy renderer");
    SDL_DestroyRenderer((*gameData)->renderer);
    (*gameData)->renderer = NULL;
    SDL_Log("destroy window");
    SDL_DestroyWindow((*gameData)->window);
    (*gameData)->window = NULL;
    SDL_Log("sdl quit");
    SDL_Quit();
    *gameData = NULL;
}