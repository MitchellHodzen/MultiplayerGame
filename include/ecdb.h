#ifndef ECDBLIB
#define ECDBLIB
#include <stdlib.h> 
#include <stdbool.h>

struct IntStack;

typedef struct ECDB
{
    unsigned int _maxEntities;
    unsigned int _maxComponents;
    unsigned int _componentCount;
    struct IntStack* _entityIdStack;
    bool* _validEntities;
    void** _componentArrays;
    bool** _componentValidArrays;
    size_t* _componentSizes;
} ECDB;

bool ECDB_Init(ECDB*const ecdb, unsigned int maxEntities, unsigned int maxComponents);
bool ECDB_RegisterComponent(ECDB*const ecdb, size_t componentSize, int* componentHandle);
bool ECDB_CreateEntity(ECDB const *const ecdb, int* entityId);
bool ECDB_EntityHasComponent(ECDB const *const ecdb, int entityId, int componentHandle);
void* ECDB_EnableEntityComponent(ECDB const *const ecdb, int entityId, int componentHandle);
void* ECDB_GetEntityComponent(ECDB const *const ecdb, int entityId, int componentHandle);
void ECDB_Free(ECDB* ecdb);

#endif /* ECDBLIB */