#ifndef ECDBLIB
#define ECDBLIB
#include <stdlib.h> 
#include <stdbool.h>

struct IntStack;

struct ECDB
{
    unsigned int _maxEntities;
    unsigned int _maxComponents;
    unsigned int _componentCount;
    struct IntStack* _entityIdStack;
    bool* _validEntities;
    void** _componentArrays;
    bool** _componentValidArrays;
    size_t* _componentSizes;
};

bool ECDB_Init(struct ECDB** ecdb, unsigned int maxEntities, unsigned int maxComponents);
bool ECDB_RegisterComponent(struct ECDB*const ecdb, size_t componentSize, int* componentHandle);
bool ECDB_CreateEntity(struct ECDB const *const ecdb, int* entityId);
bool ECDB_EntityHasComponent(struct ECDB const *const ecdb, int entityId, int componentHandle);
void* ECDB_EnableEntityComponent(struct ECDB const *const ecdb, int entityId, int componentHandle);
void* ECDB_GetEntityComponent(struct ECDB const *const ecdb, int entityId, int componentHandle);
void ECDB_Free(struct ECDB** ecdb);

#endif /* ECDBLIB */