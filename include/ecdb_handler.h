#ifndef ECDBHANDLER
#define ECDBHANDLER
#include <stdbool.h>

struct ECDB;
struct Vector2;
struct C_Input;
struct SDL_FColor;

struct ECDB_Handler
{
    struct ECDB* ecdb;
    int positions_handle;
    int colors_handle;
    int inputs_handle;
};

bool ECDB_Handler_Init(struct ECDB_Handler** ecdb_handler, unsigned int maxEntities);
void ECDB_Handler_Free(struct ECDB_Handler** ecdb_handler);

struct Vector2* ECDB_Handler_Get_Positions(struct ECDB_Handler const *const ecdb_handler);
struct C_Input* ECDB_Handler_Get_Inputs(struct ECDB_Handler const *const ecdb_handler);
struct SDL_FColor* ECDB_Handler_Get_Colors(struct ECDB_Handler const *const ecdb_handler);

bool ECDB_Handler_EntityHasPosition(struct ECDB_Handler const *const ecdb_handler, int entityId);
bool ECDB_Handler_EntityHasInput(struct ECDB_Handler const *const ecdb_handler, int entityId);
bool ECDB_Handler_EntityHasColor(struct ECDB_Handler const *const ecdb_handler, int entityId);

#endif /* ECDBHANDLER */