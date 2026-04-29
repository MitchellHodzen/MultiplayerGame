#include <stdio.h>
#include <stdlib.h>
#include <windows.h> 
#include <SDL3/SDL.H>
#include <SDL3/SDL_main.h>
#include "ecdb.h"
#include "vector2.h"
#include "system_movement.h"
#include "system_render.h"
#include "intstack.h"
#include "component_input.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ENTITY_COUNT 100

struct ECDB* ec = NULL;
int positions_handle;
int colors_handle;
int inputs_handle;

bool InitializeECDB(unsigned int entityCount)
{
    ec = (struct ECDB*) malloc(sizeof(struct ECDB));
    if (!ECDB_Init(ec, entityCount, 3))
    {
        SDL_Log("Couldn't initialize component DB");
        return false;
    }

    if (!ECDB_RegisterComponent(ec, sizeof(struct Vector2), &positions_handle))
    {
        SDL_Log("Couldn't initialize positions component");
        ECDB_Free(ec);
        return false;
    }
    if (!ECDB_RegisterComponent(ec, sizeof(SDL_FColor), &colors_handle))
    {
        SDL_Log("Couldn't initialize colors component");
        ECDB_Free(ec);
        return false;
    }
    if (!ECDB_RegisterComponent(ec, sizeof(struct C_Input), &inputs_handle))
    {
        SDL_Log("Couldn't initialize input component");
        ECDB_Free(ec);
        return false;
    }
    return true;
}

bool AddSquare(struct Vector2 position, SDL_FColor color, float speed, int* entityId)
{
    if (ECDB_CreateEntity(ec, entityId) == false)
    {
        SDL_Log("Couldn't create square");
        return false;
    }

    struct Vector2* entityPos = ECDB_EnableEntityComponent(ec, *entityId, positions_handle);
    memcpy(entityPos, &position, sizeof(struct Vector2));
    SDL_FColor* entityCol = ECDB_EnableEntityComponent(ec, *entityId, colors_handle);
    memcpy(entityCol, &color, sizeof(SDL_FColor));
    struct C_Input* entityInput = ECDB_EnableEntityComponent(ec, *entityId, inputs_handle);
    entityInput->speed=speed;
    return true;
}

bool InitializeSDL(SDL_Window** window, SDL_Renderer** renderer)
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

    if (!SDL_CreateWindowAndRenderer("gaem", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, window, renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}

struct WorkerThreadInput
{
    const bool* QuitFlag;
    int num;
};

DWORD WINAPI PrintEverySecond(struct WorkerThreadInput* input)
{
    while(*(input->QuitFlag) == false)
    {
        input->num++;
        printf("hello world %i\n", input->num);
        sleep(1);
    }

    return 0;
}

int main(int argc, char* args[])
{
    bool quit = false;
    struct WorkerThreadInput threadInput = {.num = 0, .QuitFlag = &quit};
    HANDLE threadHandle = CreateThread(NULL, 0, PrintEverySecond, &threadInput, 0, NULL);
    if (threadHandle == NULL)
    {
        SDL_Log("Worker Thread Initialization Failed");
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (InitializeSDL(&window, &renderer))
    {
        SDL_Log("SDL Initialized Successfully");
    }
    else
    {
        SDL_Log("SDL Initialization Failed");
        return 1;
    }

    if (InitializeECDB(ENTITY_COUNT))
    {
        SDL_Log("ECDB Initialized Successfully");
    }
    else
    {
        SDL_Log("ECDB Initialization Failed");
        return 1;
    }

    int playerId;
    int secondSquareId;
    AddSquare((struct Vector2){.x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT / 2}, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, 100, &playerId);
    AddSquare((struct Vector2){.x = SCREEN_WIDTH / 2 - 200, .y = SCREEN_HEIGHT / 2 - 200}, (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f}, 50, &secondSquareId);
    struct Vector2 direction = {.x = 0, .y = 0};

    SDL_Event e;
    Uint64 currentFrameTimeMs = SDL_GetTicks();
    Uint64 previousFrameTimeMs = currentFrameTimeMs;

    while( quit == false )
    {
        previousFrameTimeMs = currentFrameTimeMs;
        currentFrameTimeMs = SDL_GetTicks();
        float deltaTimeS = (float)(currentFrameTimeMs - previousFrameTimeMs) / 1000;

        while( SDL_PollEvent( &e ) == true )
        {
            if( e.type == SDL_EVENT_QUIT )
            {
                quit = true;
            }
            else if( e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == 0)
            {
                if( e.key.key == SDLK_W )
                {
                    direction.y--;
                }
                else if( e.key.key == SDLK_A )
                {
                    direction.x--;
                }
                else if( e.key.key == SDLK_S )
                {
                    direction.y++;
                }
                else if( e.key.key == SDLK_D )
                {
                    direction.x++;
                }
            }
            else if( e.type == SDL_EVENT_KEY_UP && e.key.repeat == 0)
            {
                if( e.key.key == SDLK_W )
                {
                    direction.y++;
                }
                else if( e.key.key == SDLK_A )
                {
                    direction.x++;
                }
                else if( e.key.key == SDLK_S )
                {
                    direction.y--;
                }
                else if( e.key.key == SDLK_D )
                {
                    direction.x--;
                }
            }
        }

        struct C_Input* inputs = (struct C_Input*) ec->_componentArrays[inputs_handle];
        for(unsigned int i = 0; i < ec->_maxEntities; ++i)
        {
            if(ECDB_EntityHasComponent(ec, i, inputs_handle))
            {
                inputs[i].direction.x = direction.x;
                inputs[i].direction.y = direction.y;
            }
        }

        // Clear previous render before drawing
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE ); // Black
        SDL_RenderClear(renderer);

        s_move(ec, positions_handle, inputs_handle, deltaTimeS);
        s_render(ec, positions_handle, colors_handle, renderer);

        // Draw to screen
        SDL_RenderPresent(renderer);
    }

    // Close up
    ECDB_Free(ec);
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_Quit();

    WaitForSingleObject(threadHandle, INFINITE);
    printf("final thread value: %i", threadInput.num);
    return 0;
}