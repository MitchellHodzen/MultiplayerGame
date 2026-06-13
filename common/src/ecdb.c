#include "ecdb.h"
#include <string.h>
#include "intstack.h"

static bool _Init_Entity_Tracking(struct ECDB* ecdb)
{
    ecdb->_entityIdStack = (struct IntStack*) malloc(sizeof(struct IntStack));

    if (ecdb->_entityIdStack == NULL)
    {
        // Couldn't create an entity ID stack
        return false;
    }    

    // Set up entity ID tracking
    if (!IntStack_Init(ecdb->_entityIdStack, ecdb->_maxEntities))
    {
        // Couldn't initialize index stack
        return false;
    }

    // Add IDs in reverse order so we add at 0 first
    for (int i = ecdb->_maxEntities -1; i >= 0; --i)
    {
        // Don't checdbk for failure - we've already confirmed entityCount is good in the init step
        IntStack_Push(ecdb->_entityIdStack, i);
    }

    // set up valid entity array. One extra for the invalid entity
    ecdb->_validEntities = (bool*) calloc(ecdb->_maxEntities + 1, sizeof(bool));
    if (ecdb->_validEntities == NULL)
    {
        // Couldn't create the valid entities array
        return false;
    }
    
    return true;
}

static bool _Init_Components(struct ECDB* ecdb)
{
    // Set up component ID containers
    ecdb->_componentArrays = (void**) calloc(ecdb->_maxComponents + 1, sizeof(void*));
    if (ecdb->_componentArrays == NULL)
    {
        // Couldn't create the component array container
        return false;
    }

    ecdb->_componentValidArrays = (bool**) calloc(ecdb->_maxComponents + 1, sizeof(bool*));
    if (ecdb->_componentValidArrays == NULL)
    {
        // Couldn't create the component valid array container
        return false;
    }
    
    ecdb->_componentSizes = (size_t*) calloc(ecdb->_maxComponents + 1, sizeof(size_t));
    if (ecdb->_componentSizes == NULL)
    {
        // Couldn't create the component size array
        return false;
    }

    return true;
}

bool ECDB_Init(struct ECDB** ecdb, unsigned int maxEntities, unsigned int maxComponents)
{
    *ecdb = (struct ECDB*) malloc(sizeof(struct ECDB));
    if (*ecdb == NULL)
    {
        // Couldnt instantiate ecdb
        return false;
    }
    
    (*ecdb)->_maxEntities = maxEntities;
    (*ecdb)->_maxComponents = maxComponents;
    (*ecdb)->_componentCount = 0;
    (*ecdb)->invalidEntityId = maxEntities + 1; // Invalid entity ID is at the end of the list of valid entities
    (*ecdb)->invalidComponentId = maxComponents + 1; // Invalid component ID is at the end of the list of valid components

    if (_Init_Entity_Tracking(*ecdb) == false || _Init_Components(*ecdb) == false)
    {
        ECDB_Free(ecdb);
        return false;
    }
    
    return true;
}

bool ECDB_RegisterComponent(struct ECDB* ecdb, size_t componentSize, int* componentHandle)
{
    // Check if we can add another component
    if (ecdb->_componentCount >= ecdb->_maxComponents)
    {
        *componentHandle = ecdb->invalidComponentId;
        return false;
    }

    // For both component arrays add one extra for the invalid entity
    void* componentArray = calloc(ecdb->_maxEntities + 1, componentSize);
    if (componentArray == NULL)
    {
        // couldn't instantiate the component array
        return false;
    }
    bool* componentValidArray = (bool*) calloc(ecdb->_maxEntities + 1, sizeof(bool));
    if (componentValidArray == NULL)
    {
        // couldn't instantiate the component validity array
        free(componentArray); // Since we created the component array, but total initialization failed, free it
        return false;
    }

    // Add the components array to the component array collections
    ecdb->_componentArrays[ecdb->_componentCount] = componentArray;
    ecdb->_componentValidArrays[ecdb->_componentCount] = componentValidArray;
    ecdb->_componentSizes[ecdb->_componentCount] = componentSize;

    // Now that the component has been added, return the handle.
    *componentHandle = ecdb->_componentCount;

    // increment the number of components
    ecdb->_componentCount++;

    return true;
}

bool ECDB_CreateEntity(struct ECDB* ecdb, int* entityId)
{
    // Get an entity ID from the stack
    if (!IntStack_Pop(ecdb->_entityIdStack, entityId))
    {
        *entityId = ecdb->invalidEntityId;
        return false;
    }

    // Enable the entity
    ecdb->_validEntities[*entityId] = true;
    return true;
}

void ECDB_DestroyEntity(struct ECDB* ecdb, int entityId)
{
    // Disable the entity
    ecdb->_validEntities[entityId] = false;

    // Disable all components for the entity
    for (int componentHandle = 0; componentHandle < ecdb->_maxComponents; ++componentHandle)
    {
        ecdb->_componentValidArrays[componentHandle][entityId] = false;
    }

    // Make the entity Id available for use again
    IntStack_Push(ecdb->_entityIdStack, entityId);
}

bool ECDB_EntityHasComponent(struct ECDB const *const ecdb, int entityId, int componentHandle)
{
    return ecdb->_componentValidArrays[componentHandle][entityId];
}

void* ECDB_EnableEntityComponent(struct ECDB* ecdb, int entityId, int componentHandle)
{
    if (ECDB_EntityHasComponent(ecdb, entityId, componentHandle))
    {
        // entity already has the component, return it
        return ECDB_GetEntityComponent(ecdb, entityId, componentHandle);
    }

    // Zero out the component in the array that exists already
    void* componentArray = ecdb->_componentArrays[componentHandle];
    size_t componentSize = ecdb->_componentSizes[componentHandle];
    void* component = ((char*)componentArray) + (entityId * componentSize);
    memset(component, 0, componentSize);

    // Set the entity to have the component
    ecdb->_componentValidArrays[componentHandle][entityId] = true;
    return component;
}

void* ECDB_GetEntityComponent(struct ECDB const *const ecdb, int entityId, int componentHandle)
{
    // Get a pointer to the component in the components array
    void* componentArray = ecdb->_componentArrays[componentHandle];
    size_t componentSize = ecdb->_componentSizes[componentHandle];
    return ((char*)componentArray) + (entityId * componentSize);
}

void ECDB_Free(struct ECDB** ecdb)
{
    IntStack_Free((*ecdb)->_entityIdStack);
    for(unsigned int i = 0; i < (*ecdb)->_componentCount; ++i)
    {
        free((*ecdb)->_componentArrays[i]);
        free((*ecdb)->_componentValidArrays[i]);
    }
    free((*ecdb)->_componentArrays);
    free((*ecdb)->_componentValidArrays);
    free((*ecdb)->_componentSizes);
    free((*ecdb)->_validEntities);
    free(*ecdb);
    *ecdb = NULL;
}