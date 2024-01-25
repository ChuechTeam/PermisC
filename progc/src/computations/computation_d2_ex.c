#include "computations.h"
#include <stdlib.h>
#include <stdio.h>
#include "compile_settings.h"

#if EXPERIMENTAL_ALGO
#include <string.h>

#include "route.h"
#include "avl.h"
#include "profile.h"
#include "mem_alloc.h"
#include "map.h"

static MemArena driverSortAVLMem;
static MemArena driverStringsMem;

/*
 * Driver Map
 */

typedef struct
{
    char* str;
    uint32_t length;
} MeasuredString;

typedef struct DriverEntry
{
    char* name; // NULL if empty.
    uint32_t length;
    float dist;
} DriverEntry;

typedef struct DriverMap
{
    MAP_HEADER(DriverEntry)
} DriverMap;

#define CURRENT_MAP_TYPE() DriverMap

static inline uint32_t MAP_HASH_FUNC(const MeasuredString* key, uint32_t capacityExponent)
{
    uint32_t a = 0;
    for (uint32_t i = 0; i < key->length; i++)
    {
        a *= 31;
        a += key->str[i];
    }
    return a;
}

static inline bool MAP_KEY_EQUAL_FUNC(const DriverEntry* entry, const MeasuredString* key)
{
    return entry->length == key->length && memcmp(entry->name, key->str, key->length) == 0;
}

static inline bool MAP_GET_OCCUPIED_FUNC(const DriverEntry* entry)
{
    return entry->name != NULL;
}

static inline void MAP_MARK_OCCUPIED_FUNC(DriverEntry* entry, MeasuredString* key)
{
    char* copy = memAlloc(&driverStringsMem, key->length + 1);
    memcpy(copy, key->str, key->length + 1);

    entry->name = copy;
    entry->length = key->length;
}

static inline MeasuredString* MAP_GET_KEY_PTR_FUNC(DriverEntry* entry, MapKeyScratch scratch)
{
    MeasuredString* scratchStr = (MeasuredString*) scratch;

    scratchStr->str = entry->name;
    scratchStr->length = entry->length;
    return scratchStr;
}

MAP_DECLARE_FUNCTIONS_STATIC(driverMap, DriverEntry, MeasuredString, true)

#undef CURRENT_MAP_TYPE

typedef struct DriverSortAVL
{
    AVL_HEADER(DriverSortAVL)

    DriverEntry* driver;
} DriverSortAVL;

static DriverSortAVL* driverSortAVLCreate(DriverEntry* driver)
{
    DriverSortAVL* tree = memAlloc(&driverSortAVLMem, sizeof(DriverSortAVL));

    AVL_INIT(tree);
    tree->driver = driver;

    return tree;
}

static int driverSortAVLCompare(DriverSortAVL* tree, DriverEntry* driver)
{
    if (tree->driver->dist > driver->dist)
    {
        return 1;
    }
    else if (tree->driver->dist < driver->dist)
    {
        return -1;
    }
    else
    {
        return strcmp(tree->driver->name, driver->name);
    }
}

AVL_DECLARE_FUNCTIONS_STATIC(driverSortAVL, DriverSortAVL, DriverEntry,
                             (AVLCreateFunc) &driverSortAVLCreate, (AVLCompareValueFunc) &driverSortAVLCompare)

static void sortDrivers(DriverMap* drivers, DriverSortAVL** sorted)
{
    for (uint32_t i = 0; i < drivers->capacity; ++i)
    {
        if (drivers->entries[i].name != NULL)
        {
            *sorted = driverSortAVLInsert(*sorted, &drivers->entries[i], NULL, NULL);
        }
    }
}

static void printTop10(DriverSortAVL* tree, int* n)
{
    if (tree == NULL || *n >= 10)
    {
        return;
    }

    printTop10(tree->right, n);

    if (*n < 10)
    {
        printf("%s;%f\n", tree->driver->name, tree->driver->dist);
        *n += 1;
    }

    printTop10(tree->left, n);
}

void computationD2(RouteStream* stream)
{
    PROFILER_START("Computation D2");

    DriverMap drivers;
    driverMapInit(&drivers, 4096, 0.75f);

    memInit(&driverSortAVLMem, 128 * 1024);
    memInit(&driverStringsMem, 256 * 1024);

    RouteStep step;
    while (rsRead(stream, &step, DRIVER_NAME | DISTANCE))
    {
        MeasuredString drivStr = { step.driverName, step.driverNameLen };

        DriverEntry* driver = driverMapLookup(&drivers, drivStr);
        if (driver == NULL)
        {
            driver = driverMapInsert(&drivers, drivStr);
        }

        driver->dist += step.distance;
    }

    DriverSortAVL* sorted = NULL;
    int n = 0;

    sortDrivers(&drivers, &sorted);
    printTop10(sorted, &n);

    driverMapFree(&drivers);
    memFree(&driverSortAVLMem);
    memFree(&driverStringsMem);

    PROFILER_END();
}

#else
void computationD2(struct RouteStream* s)
{
    fprintf(stderr, "Cannot use computation D2 without experimental algorithms enabled!\n");
    exit(9);
}
#endif
