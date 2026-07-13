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
    ecdb->_entityIdStack = (struct IntStack*) malloc(IntStack_Calculate_Required_Memory(ecdb->_maxEntities));
    if (ecdb->_entityIdStack == NULL)
    {
        // Couldn't allocate index stack
        return false;
    }

    IntStack_Init(ecdb->_entityIdStack, ecdb->_maxEntities);

    // Add IDs in reverse order so we add at 0 first
    for (int i = ecdb->_maxEntities -1; i >= 0; --i)
    {
        // Don't checdbk for failure - we've already confirmed entityCount is good in the init step
        IntStack_Push(ecdb->_entityIdStack, i);
    }

    // set up valid entity array. One extra for the invalid entity
    ecdb->validEntities = (bool*) calloc(ecdb->_maxEntities + 1, sizeof(bool));
    if (ecdb->validEntities == NULL)
    {
        // Couldn't create the valid entities array
        return false;
    }
    
    return true;
}

static bool _Init_Components(struct ECDB* ecdb)
{
    // Set up component ID containers
    ecdb->componentArrays = (void**) calloc(ecdb->_maxComponents + 1, sizeof(void*));
    if (ecdb->componentArrays == NULL)
    {
        // Couldn't create the component array container
        return false;
    }

    ecdb->componentValidArrays = (bool**) calloc(ecdb->_maxComponents + 1, sizeof(bool*));
    if (ecdb->componentValidArrays == NULL)
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
        // Store the default value at the invalid entity, as that should never change
        void* comp = ((char*)componentArray) + (ecdb->invalidEntityId * componentSize);
        memcpy(comp, defaultValue, componentSize);
    }

    // Add the components array to the component array collections
    ecdb->componentArrays[ecdb->_componentCount] = componentArray;
    ecdb->componentValidArrays[ecdb->_componentCount] = componentValidArray;
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
    ecdb->validEntities[*entityId] = true;
    return true;
}

void ECDB_DestroyEntity(struct ECDB* ecdb, int entityId)
{
    // Disable the entity
    ecdb->validEntities[entityId] = false;

    // Disable all components for the entity
    for (unsigned int componentHandle = 0; componentHandle < ecdb->_maxComponents; ++componentHandle)
    {
        ECDB_DisableEntityComponent(ecdb, entityId, componentHandle);
    }

    // Make the entity Id available for use again
    IntStack_Push(ecdb->_entityIdStack, entityId);
}

bool ECDB_EntityHasComponent(struct ECDB const *const ecdb, int entityId, int componentHandle)
{
    return ecdb->componentValidArrays[componentHandle][entityId];
}

void* ECDB_EnableEntityComponent(struct ECDB* ecdb, int entityId, int componentHandle)
{
    if (ECDB_EntityHasComponent(ecdb, entityId, componentHandle))
    {
        // entity already has the component, return it
        return ECDB_GetEntityComponent(ecdb, entityId, componentHandle);
    }

    // Get a pointer to the component
    void* componentArray = ecdb->componentArrays[componentHandle];
    size_t componentSize = ecdb->_componentSizes[componentHandle];
    void* component = ((char*)componentArray) + (entityId * componentSize);

    // Set the component value to the default, which is held at the invalidEntity index
    void* defaultComponent = ((char*)componentArray) + (ecdb->invalidEntityId * componentSize);
    memcpy(component, defaultComponent, componentSize);

    // Set the entity to have the component
    ecdb->componentValidArrays[componentHandle][entityId] = true;
    return component;
}

void ECDB_DisableEntityComponent(struct ECDB* ecdb, int entityId, int componentHandle)
{
    // Disable the component for the entity
    ecdb->componentValidArrays[componentHandle][entityId] = false;
}

void* ECDB_GetEntityComponent(struct ECDB const *const ecdb, int entityId, int componentHandle)
{
    // Get a pointer to the component in the components array
    void* componentArray = ecdb->componentArrays[componentHandle];
    size_t componentSize = ecdb->_componentSizes[componentHandle];
    return ((char*)componentArray) + (entityId * componentSize);
}

size_t ECDB_Bool_Array_Size(struct ECDB const *const ecdb)
{
    // valid entities is an array of max entities + 1 bools
    return (ecdb->_maxEntities + 1) * sizeof(bool);
}

size_t ECDB_Snapshot_Size(struct ECDB const *const ecdb)
{
    size_t retval = 0;

    // Any entity assigned data array will be max entities + 1 for the invalid entity
    unsigned int array_size = ecdb->_maxEntities + 1;
    
    // valid entities is just a bool array
    retval += ECDB_Bool_Array_Size(ecdb);

    // component valid arrays is a max entities + 1 bool array for each component
    retval += ECDB_Bool_Array_Size(ecdb) * ecdb->_componentCount;

    // component arrays is a max entities + 1 array for each component, where each element of each array is the sizeof the component
    for(unsigned int i = 0; i < ecdb->_componentCount; ++i)
    {
        size_t component_size = ecdb->_componentSizes[i];
        retval += array_size * component_size;
    }

    return retval;
}

void ECDB_Generate_Snapshot(struct ECDB const *const ecdb, void* snapshot)
{
    // Snapshot is a contiguous array of entity valid data, component valid data, and component data, in that order
    // Assume the passed in snapshot has already allocated at least ECDB_Snapshot_Size memory

    // Copy the valid entities array at the start of the snapshot
    size_t bool_array_size = ECDB_Bool_Array_Size(ecdb);
    memcpy(snapshot, ecdb->validEntities, bool_array_size);
    /*printf("from array: ");
    for(int i = 0; i < bool_array_size; ++i)
    {
        printf("%i ", ecdb->validEntities[i]);
    }
    printf("\n");

    printf("  to array: ");
    for(int i = 0; i < bool_array_size; ++i)
    {
        printf("%i ", ((bool*)snapshot)[i]);
    }
    printf("\n");*/

    snapshot = (char*)snapshot + bool_array_size;

    // Copy the valid component arrays next
    for(unsigned int i = 0; i < ecdb->_componentCount; ++i)
    {
        memcpy(snapshot, ecdb->componentValidArrays[i], bool_array_size);

        /*printf("from array: ", i);
        for(int j = 0; j < bool_array_size; ++j)
        {
            printf("%i ", ecdb->componentValidArrays[i][j]);
        }
        printf("\n");

        printf("  to array: ", i);
        for(int j = 0; j < bool_array_size; ++j)
        {
            printf("%i ", ((bool*)snapshot)[j]);
        }
        printf("\n");*/

        snapshot = (char*)snapshot + bool_array_size;

    }

    // Lastly, copy over all actual component data
    for(unsigned int i = 0; i < ecdb->_componentCount; ++i)
    {
        // Each component array is max entities + 1 of the size of the component
        size_t arr_len = (ecdb->_maxEntities + 1) * ecdb->_componentSizes[i];
        memcpy(snapshot, ecdb->componentArrays[i], arr_len);

        /*for(int j = 0; j < arr_len; ++j)
        {
            unsigned char original_byte = ((unsigned char*)(ecdb->componentArrays[i]))[j];
            unsigned char new_byte = ((unsigned char*)snapshot)[j];
            if (original_byte != new_byte)
            {
                printf("there's a problem\n");
            }
        }*/

        snapshot = (char*)snapshot + arr_len;
    }
}

void ECDB_Apply_Snapshot(struct ECDB* ecdb, void* snapshot)
{
    // Copy the valid entities array from the start of the snapshot to the valid entities array
    size_t bool_array_size = ECDB_Bool_Array_Size(ecdb);
    memcpy(ecdb->validEntities, snapshot, bool_array_size);
    snapshot = (char*)snapshot + bool_array_size;

    // Copy the valid component arrays from the snapshot to the ecdb
    for(unsigned int i = 0; i < ecdb->_componentCount; ++i)
    {
        memcpy(ecdb->componentValidArrays[i], snapshot, bool_array_size);
        snapshot = (char*)snapshot + bool_array_size;
    }

    // Lastly, copy over all actual component data
    for(unsigned int i = 0; i < ecdb->_componentCount; ++i)
    {
        // Each component array is max entities + 1 of the size of the component
        size_t arr_len = (ecdb->_maxEntities + 1) * ecdb->_componentSizes[i];
        memcpy(ecdb->componentArrays[i], snapshot, arr_len);
        snapshot = (char*)snapshot + arr_len;
    }
}

void ECDB_Free(struct ECDB** ecdb)
{
    free((*ecdb)->_entityIdStack);
    for(unsigned int i = 0; i < (*ecdb)->_componentCount; ++i)
    {
        free((*ecdb)->componentArrays[i]);
        free((*ecdb)->componentValidArrays[i]);
    }
    free((*ecdb)->componentArrays);
    free((*ecdb)->componentValidArrays);
    free((*ecdb)->_componentSizes);
    free((*ecdb)->validEntities);
    free(*ecdb);
    *ecdb = NULL;
}