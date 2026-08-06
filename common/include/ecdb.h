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
    size_t* _componentSizes;
    bool* validEntities;
    void** componentArrays;
    bool** componentValidArrays;
    unsigned int invalidEntityId;
    unsigned int invalidComponentId;
};

bool ECDB_Init(struct ECDB** ecdb, unsigned int maxEntities, unsigned int maxComponents);
bool ECDB_RegisterComponent(struct ECDB* ecdb, size_t componentSize, int* componentHandle, void* defaultValue);
bool ECDB_CreateEntity(struct ECDB* ecdb, unsigned int* entityId);
void ECDB_DestroyEntity(struct ECDB* ecdb, unsigned int entityId);
bool ECDB_EntityHasComponent(struct ECDB const *const ecdb, unsigned int entityId, int componentHandle);
void* ECDB_EnableEntityComponent(struct ECDB* ecdb, unsigned int entityId, int componentHandle);
void ECDB_DisableEntityComponent(struct ECDB* ecdb, unsigned int entityId, int componentHandle);
void* ECDB_GetEntityComponent(struct ECDB const *const ecdb, unsigned int entityId, int componentHandle);
size_t ECDB_Snapshot_Size(struct ECDB const *const ecdb);
void ECDB_Generate_Snapshot(struct ECDB const *const ecdb, void* snapshot);
void ECDB_Apply_Snapshot(struct ECDB* ecdb, void* snapshot);
void ECDB_Free(struct ECDB** ecdb);

#endif /* ECDBLIB */