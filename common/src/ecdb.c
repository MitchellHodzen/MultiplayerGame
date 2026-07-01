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
    ecdb->data.validEntities = (bool*) calloc(ecdb->_maxEntities + 1, sizeof(bool));
    if (ecdb->data.validEntities == NULL)
    {
        // Couldn't create the valid entities array
        return false;
    }
    
    return true;
}

static bool _Init_Components(struct ECDB* ecdb)
{
    // Set up component ID containers
    ecdb->data.componentArrays = (void**) calloc(ecdb->_maxComponents + 1, sizeof(void*));
    if (ecdb->data.componentArrays == NULL)
    {
        // Couldn't create the component array container
        return false;
    }

    ecdb->data.componentValidArrays = (bool**) calloc(ecdb->_maxComponents + 1, sizeof(bool*));
    if (ecdb->data.componentValidArrays == NULL)
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
    (*ecdb)->invalidEntityId = maxEntities; // Invalid entity ID is at the end of the list of valid entities
    (*ecdb)->invalidComponentId = maxComponents; // Invalid component ID is at the end of the list of valid components

    if (_Init_Entity_Tracking(*ecdb) == false || _Init_Components(*ecdb) == false)
    {
        ECDB_Free(ecdb);
        return false;
    }
    
    return true;
}

bool ECDB_RegisterComponent(struct ECDB* ecdb, size_t componentSize, int* componentHandle, void* defaultValue)
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
        printf("cant alloc\n", ecdb->_maxEntities, componentSize);
        *componentHandle = ecdb->invalidComponentId;
        return false;
    }
    bool* componentValidArray = (bool*) calloc(ecdb->_maxEntities + 1, sizeof(bool));
    if (componentValidArray == NULL)
    {
        // couldn't instantiate the component validity array
        free(componentArray); // Since we created the component array, but total initialization failed, free it
        *componentHandle = ecdb->invalidComponentId;
        return false;
    }

    // If there is a default value provided, save it
    if (defaultValue != NULL)
    {
        printf("sizeof component: %i. invalid entity id: %i. Offset: %i\n", componentSize, ecdb->invalidEntityId, (ecdb->invalidEntityId * componentSize));
        // Store the default value at the invalid entity, as that should never change
        void* comp = ((char*)componentArray) + (ecdb->invalidEntityId * componentSize);
        memcpy(comp, defaultValue, componentSize);
    }

    // Add the components array to the component array collections
    ecdb->data.componentArrays[ecdb->_componentCount] = componentArray;
    ecdb->data.componentValidArrays[ecdb->_componentCount] = componentValidArray;
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
    ecdb->data.validEntities[*entityId] = true;
    return true;
}

void ECDB_DestroyEntity(struct ECDB* ecdb, int entityId)
{
    // Disable the entity
    ecdb->data.validEntities[entityId] = false;

    // Disable all components for the entity
    for (int componentHandle = 0; componentHandle < ecdb->_maxComponents; ++componentHandle)
    {
        ECDB_DisableEntityComponent(ecdb, entityId, componentHandle);
    }

    // Make the entity Id available for use again
    IntStack_Push(ecdb->_entityIdStack, entityId);
}

bool ECDB_EntityHasComponent(struct ECDB const *const ecdb, int entityId, int componentHandle)
{
    return ecdb->data.componentValidArrays[componentHandle][entityId];
}

void* ECDB_EnableEntityComponent(struct ECDB* ecdb, int entityId, int componentHandle)
{
    if (ECDB_EntityHasComponent(ecdb, entityId, componentHandle))
    {
        // entity already has the component, return it
        return ECDB_GetEntityComponent(ecdb, entityId, componentHandle);
    }

    // Get a pointer to the component
    void* componentArray = ecdb->data.componentArrays[componentHandle];
    size_t componentSize = ecdb->_componentSizes[componentHandle];
    void* component = ((char*)componentArray) + (entityId * componentSize);

    // Set the component value to the default, which is held at the invalidEntity index
    void* defaultComponent = ((char*)componentArray) + (ecdb->invalidEntityId * componentSize);
    memcpy(component, defaultComponent, componentSize);

    // Set the entity to have the component
    ecdb->data.componentValidArrays[componentHandle][entityId] = true;
    return component;
}

void ECDB_DisableEntityComponent(struct ECDB* ecdb, int entityId, int componentHandle)
{
    // Disable the component for the entity
    ecdb->data.componentValidArrays[componentHandle][entityId] = false;
}

void* ECDB_GetEntityComponent(struct ECDB const *const ecdb, int entityId, int componentHandle)
{
    // Get a pointer to the component in the components array
    void* componentArray = ecdb->data.componentArrays[componentHandle];
    size_t componentSize = ecdb->_componentSizes[componentHandle];
    return ((char*)componentArray) + (entityId * componentSize);
}

void ECDB_Free(struct ECDB** ecdb)
{
    IntStack_Free((*ecdb)->_entityIdStack);
    for(unsigned int i = 0; i < (*ecdb)->_componentCount; ++i)
    {
        free((*ecdb)->data.componentArrays[i]);
        free((*ecdb)->data.componentValidArrays[i]);
    }
    free((*ecdb)->data.componentArrays);
    free((*ecdb)->data.componentValidArrays);
    free((*ecdb)->_componentSizes);
    free((*ecdb)->data.validEntities);
    free(*ecdb);
    *ecdb = NULL;
}