#ifndef ENTITY_BUILDERS_DEF
#define ENTITY_BUILDERS_DEF
#include <stdbool.h>
#include "Vector2.h"

struct ECDB;
struct Component_Handles;
struct SDL_FColor;

bool AddParentedText(struct ECDB* ec, struct Component_Handles* componentHandles, unsigned int parentId, struct Vector2 position, char* input_str, int* entityId);
bool AddParentedTextWithLifetime(struct ECDB* ec, struct Component_Handles* componentHandles, unsigned int parentId, struct Vector2 position, char* input_str, float lifetimeS, int* entityId);
bool AddSquare(struct ECDB* ec, struct Component_Handles* componentHandles, struct Vector2 position, struct SDL_FColor color, int* entityId, char* playerName);

#endif /* ENTITY_BUILDERS_DEF */