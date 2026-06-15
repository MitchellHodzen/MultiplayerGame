#include "initialization.h"
#include "component_handles.h"
#include <SDL3/SDL.H>
#include <SDL3_ttf/SDL_ttf.h>
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_transform.h"
#include <clay.h>
#include "component_lifetime.h"

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount, unsigned int max_chat_size)
{
    if (!ECDB_Init(ecdb, entityCount, 5))
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

    if (!ECDB_RegisterComponent(*ecdb, sizeof(char) * (max_chat_size + 1), &(componentHandles->text_handle), NULL))
    {
        SDL_Log("Couldn't initialize text component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Lifetime), &(componentHandles->lifetimes_handle), NULL))
    {
        SDL_Log("Couldn't initialize lifetime component");
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

bool InitializeNetworkTracking(struct ECDB* ecdb, unsigned int** entityNetworkIdMap, unsigned int** networkIdEntityMap)
{
    unsigned int mapSize = sizeof(unsigned int) * (ecdb->_maxEntities + 1); // Add 1 for the invalid index
    *entityNetworkIdMap = malloc(mapSize);
    *networkIdEntityMap = malloc(mapSize);
    if (*entityNetworkIdMap == NULL || *networkIdEntityMap == NULL)
    {
        printf("Allocation of entity tracking data structures failed\n");
        return false;
    }

    // for each network ID, set the value to the invalid value
    for(unsigned int i = 0; i < mapSize; i++)
    {
        (*entityNetworkIdMap)[i] = ecdb->invalidEntityId;
        (*networkIdEntityMap)[i] = ecdb->invalidEntityId;
    }
}

bool Game_Data_Init(struct Game_Data** gameData, struct Init_Vars* init_vars, Clay_ErrorHandler errorHandler, Clay_Dimensions (*measureTextFunction)(Clay_StringSlice text, Clay_TextElementConfig *config))
{
    *gameData = (struct Game_Data*) malloc(sizeof(struct Game_Data));
    if (*gameData == NULL)
    {
        SDL_Log("Could not allocate game data struct");
        return false;
    }

    if (InitializeSDL(&(*gameData)->window, &(*gameData)->renderer, &(*gameData)->textEngine, init_vars->screen_width, init_vars->screen_height))
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

    if (InitializeClay(&(*gameData)->clayArena, (*gameData)->font, init_vars->screen_width, init_vars->screen_height, errorHandler, measureTextFunction))
    {
        SDL_Log("Clay Initialized");
    }
    else
    {
        SDL_Log("Clay Initialization Failed");
        return false;
    }

    if (InitializeECDB(&(*gameData)->ec, &(*gameData)->componentHandles, init_vars->max_entities, init_vars->max_chat_size))
    {
        SDL_Log("ECDB Initialized");
    }
    else
    {
        SDL_Log("ECDB Initialization Failed");
        return false;
    }

    if (InitializeNetworkTracking((*gameData)->ec, &(*gameData)->entityNetworkIdMap, &(*gameData)->networkIdEntityMap))
    {
        SDL_Log("Network Entity Tracking Initialized");
    }
    else
    {
        SDL_Log("Network Entity Tracking Initialization Failed");
        return false;
    }
    return true;
}

void Game_Data_Free(struct Game_Data** gameData)
{
    // Close up
    SDL_Log("free network entity tracking");
    free((*gameData)->entityNetworkIdMap);
    free((*gameData)->networkIdEntityMap);

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