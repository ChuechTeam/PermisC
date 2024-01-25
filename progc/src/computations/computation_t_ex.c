#include "compile_settings.h"

#if EXPERIMENTAL_ALGO

#include "computations.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "avl.h"
#include "route.h"
#include "mem_alloc.h"
#include "profile.h"
#include "map.h"
#include "partition.h"

/*
 * [EXPERIMENTAL!] Computation T implementation
 * Featuring: The experimental map structure, and the memory arena allocator!
 */

// The memory arenas used for each kind of structure.
// Currently those are global variables, should these be passed to the create functions instead?
MemArena townSortAVLMem; // Used to allocate TownSortAVL nodes.
MemArena townNodeListMem; // Used to allocate the TownNodeList structures (only the ones we need to allocate).
MemArena townStringsMem;

// Can be changed to uint16_t for 2x more towns stored, but limits the total amount of towns to 65536.
typedef uint32_t TownNodeId;

#define TN_LIST_SIZE 104 // Make it so RouteEntry is 128 bytes.
#define TN_LIST_NUM TN_LIST_SIZE/sizeof(TownNodeId)

typedef struct TownNodeList
{
    struct TownNodeList* next;
    uint32_t size;
    TownNodeId nodes[TN_LIST_NUM];
} TownNodeList;

void tnListInit(TownNodeList* list)
{
    list->size = 0;
    list->next = NULL;
}

TownNodeList* tnListCreate()
{
    TownNodeList* list = memAlloc(&townNodeListMem, sizeof(TownNodeList));
    assert(list);

    tnListInit(list);

    return list;
}

void tnListAdd(TownNodeList* list, TownNodeId id)
{
    if (list->size == TN_LIST_NUM - 1)
    {
        if (!list->next)
        {
            list->next = tnListCreate();
        }
        tnListAdd(list->next, id);
    }
    else
    {
        list->nodes[list->size++] = id;
    }
}

bool tnListSearch(const TownNodeList* list, TownNodeId id)
{
    for (uint32_t i = 0; i < list->size; ++i)
    {
        if (list->nodes[i] == id)
        {
            return true;
        }
    }
    if (list->next)
    {
        return tnListSearch(list->next, id);
    }
    else
    {
        return false;
    }
}

/*
 * The Route Map: linking each route to its list of visited towns.
 */

typedef struct RouteEntry
{
    // We don't use a bit field here because of TownNodeList's alignment.
    uint32_t id;
    bool occupied;
    TownNodeList towns;
} RouteEntry;

typedef struct
{
    MAP_HEADER(RouteEntry)
} RouteMap;

#define CURRENT_MAP_TYPE() RouteMap

static inline uint32_t MAP_HASH_FUNC(const int* key, uint32_t capacityExponent)
{
    uint32_t a = *(uint32_t*) key;
    a *= 2654435769U;
    return a >> (32 - capacityExponent);
}

static inline bool MAP_KEY_EQUAL_FUNC(const RouteEntry* entry, const int* key)
{
    return entry->id == *key;
}

static inline bool MAP_GET_OCCUPIED_FUNC(const RouteEntry* entry)
{
    return entry->occupied;
}

static inline void MAP_MARK_OCCUPIED_FUNC(RouteEntry* entry, int* key)
{
    entry->occupied = true;
    entry->id = *key;
}

static inline uint32_t* MAP_GET_KEY_PTR_FUNC(RouteEntry* entry, MapKeyScratch scratch)
{
    return &entry->id;
}

MAP_DECLARE_FUNCTIONS_STATIC(routeMap, RouteEntry, int, true)

#undef CURRENT_MAP_TYPE

/*
 * Town Map: Links each town id to its town name.
 */

typedef struct
{
    char* str;
    uint32_t length;
} MeasuredString;

typedef struct TownMapEntry
{
    TownNodeId id;
    bool occupied;
    uint16_t length;
    char* name;
} TownMapEntry;

typedef struct
{
    MAP_HEADER(TownMapEntry)
} TownMap;

#define CURRENT_MAP_TYPE() TownMap

static inline uint32_t MAP_HASH_FUNC(MeasuredString* key, uint32_t capacityExponent)
{
    char* str = key->str;

    uint32_t hash = 0;
    while (*str != '\0')
    {
        hash *= 31;
        hash += *str;
        str++;
    }
    return hash;
}

static inline bool MAP_KEY_EQUAL_FUNC(const TownMapEntry* entry, MeasuredString* key)
{
    if (key->length == entry->length)
    {
        return memcmp(key->str, entry->name, key->length) == 0;
    }
    else
    {
        return false;
    }
}

static inline bool MAP_GET_OCCUPIED_FUNC(const TownMapEntry* entry)
{
    return entry->occupied;
}

static inline void MAP_MARK_OCCUPIED_FUNC(TownMapEntry* entry, MeasuredString* key)
{
    size_t length = key->length;
    size_t size = length + 1;

    assert(size <= UINT16_MAX);

    entry->name = memAlloc(&townStringsMem, size);
    memcpy(entry->name, key->str, size);

    entry->length = (uint16_t) length;
    entry->occupied = true;
}

static inline MeasuredString* MAP_GET_KEY_PTR_FUNC(TownMapEntry* entry, MapKeyScratch scratch)
{
    MeasuredString* str = (MeasuredString*) scratch;
    str->str = entry->name;
    str->length = entry->length;
    return str;
}

MAP_DECLARE_FUNCTIONS_STATIC(townMap, TownMapEntry, MeasuredString, true)

#undef CURRENT_MAP_TYPE

/*
 * Town Stats, and its array
 */

typedef struct TownStats
{
    char* name;
    uint32_t passed;
    uint32_t firstTown;
} TownStats;

typedef struct TownStatsArray
{
    TownStats* elements;
    uint32_t capacity;
} TownStatsArray;

void townStatsArrayInit(TownStatsArray* array)
{
    array->capacity = 8192;
    array->elements = malloc(sizeof(TownStats) * array->capacity);
}

void townStatsArrayPut(TownStatsArray* array, TownNodeId index, const TownStats stats)
{
    if (index >= array->capacity)
    {
        array->capacity *= 2;
        array->elements = realloc(array->elements, sizeof(TownStats) * array->capacity);
    }
    array->elements[index] = stats;
}

/*
 * Town Sort AVL
 */

typedef struct TownSortAVL
{
    AVL_HEADER(TownSortAVL)

    TownStats stats;
} TownSortAVL;

TownSortAVL* townSortAVLCreate(TownStats* stats)
{
    TownSortAVL* tree = memAlloc(&townSortAVLMem, sizeof(TownSortAVL));
    // assert(tree);

    AVL_INIT(tree);
    tree->stats = *stats;

    return tree;
}

int townSortAVLComparePassed(TownSortAVL* tree, TownStats* entry)
{
    int cmp = tree->stats.passed - entry->passed;
    if (cmp != 0)
    {
        return cmp;
    }
    else
    {
        return strcmp(tree->stats.name, entry->name);
    }
}

int townSortAVLCompareName(TownSortAVL* tree, TownStats* townNode)
{
    return strcmp(tree->stats.name, townNode->name);
}

AVL_DECLARE_INSERT_FUNCTION(townSortAVLInsertPassed, TownSortAVL, TownStats,
                            (AVLCreateFunc) &townSortAVLCreate, (AVLCompareValueFunc) &townSortAVLComparePassed)

AVL_DECLARE_INSERT_FUNCTION(townSortAVLInsertName, TownSortAVL, TownStats,
                            (AVLCreateFunc) &townSortAVLCreate, (AVLCompareValueFunc) &townSortAVLCompareName)


typedef struct StepPart
{
    uint32_t routeId;
    TownNodeId townA;
    TownNodeId townB;
} StepPart;

static inline TownNodeId registerTown(const RouteStep* step, TownMap* towns, TownStatsArray* statArray,
                                      MeasuredString townName, TownNodeId* idCounter)
{
    TownMapEntry* townNode = townMapLookup(towns, townName);
    if (townNode == NULL)
    {
        townNode = townMapInsert(towns, townName);
        townNode->id = (*idCounter)++;
        townStatsArrayPut(statArray, townNode->id, (TownStats){townNode->name, 0, 0});
    }

    if (step->stepId == 1 && townName.str == step->townA)
    {
        statArray->elements[townNode->id].firstTown++;
    }

    return townNode->id;
}

static inline void incrementTownPassed(RouteEntry* entry, TownStatsArray* statArray, TownNodeId townId)
{
    TownStats* stats = &statArray->elements[townId];

    if (!tnListSearch(&entry->towns, townId))
    {
        stats->passed++;
        tnListAdd(&entry->towns, townId);
    }
}

void sortTowns(TownStatsArray* stats, TownNodeId num, TownSortAVL** sorted)
{
    for (uint32_t i = 0; i < num; ++i)
    {
        TownStats* stat = &stats->elements[i];
        *sorted = townSortAVLInsertPassed(*sorted, stat, NULL, NULL);
    }
}

void extractTop10(TownSortAVL* sorted, TownSortAVL** top, int* n)
{
    if (sorted == NULL || *n == 10)
    {
        return;
    }

    extractTop10(sorted->right, top, n);

    if (*n != 10)
    {
        *top = townSortAVLInsertName(*top, &sorted->stats, NULL, NULL);
        *n += 1;
    }

    extractTop10(sorted->left, top, n);
}

void printTop10(TownSortAVL* top)
{
    if (top == NULL)
    {
        return;
    }

    printTop10(top->left);
    printf("%s;%d;%d\n", top->stats.name, top->stats.passed, top->stats.firstTown);
    printTop10(top->right);
}

void computationT(RouteStream* stream)
{
    PROFILER_START("Computation T (Experimental!)");

    memInit(&townSortAVLMem, 256 * 1024);
    memInit(&townNodeListMem, 1 * 1024 * 1024);
    memInitEx(&townStringsMem, 512 * 1024, 1);

    // Stores all the towns travelled in each route.
    RouteMap routes;
    // Stores the identifiers of the towns, and their name as strings.
    TownMap towns;
    // Stores the passed/first-passed stats of each town.
    // It's an array, but it's clearly accessed like a map, because the ids are sequential.
    TownStatsArray stats;
    // Writes all the steps into partitions, grouping them into
    // batches of steps with the same route id.
    // This improves performance a LOT by reducing cache misses, as the algorithm will
    // access the same routes more frequently, instead of systematically
    // accessing a random area in the RAM.
    Partitioner partitioner;
    // Just tracks the current town id, and increments it for the next town we encounter.
    TownNodeId idCounter = 0;

    routeMapInit(&routes, 8192, 0.25f); // Low load factor because we have low cardinality
    // A lower load factor is better for this map as strings are really just stored
    // in another memory region, and the key bottleneck is comparing strings; we must
    // then reduce the number of collisions as much as possible.
    townMapInit(&towns, 8192, 0.5f);
    // More partitions is efficent for this computation as the route map is very large.
    partitionerInit(&partitioner, 128, 65536);
    townStatsArrayInit(&stats);

    {
        PROFILER_START("Write to partitions + town registering");

        RouteStep step;
        while (rsRead(stream, &step, ROUTE_ID | STEP_ID | TOWN_A | TOWN_B))
        {
            StepPart part = {step.routeId};
            part.townA = registerTown(&step, &towns, &stats, (MeasuredString){step.townA, step.townALen}, &idCounter);
            part.townB = registerTown(&step, &towns, &stats, (MeasuredString){step.townB, step.townBLen}, &idCounter);
            partinitionerAddS(&partitioner, step.routeId, part);
        }

        PROFILER_END();
    }

    {
        PROFILER_START("Read partitioned entries");

        for (uint32_t i = 0; i < partitioner.numPartitions; ++i)
        {
            Partition* partition = &partitioner.partitions[i];

            PARTITION_ITERATE(&partitioner, partition, StepPart, stepPart)
            {
                RouteEntry* entry = routeMapLookup(&routes, stepPart->routeId);
                if (entry == NULL)
                {
                    entry = routeMapInsert(&routes, stepPart->routeId);
                    tnListInit(&entry->towns);
                }

                incrementTownPassed(entry, &stats, stepPart->townA);
                incrementTownPassed(entry, &stats, stepPart->townB);
            }

            routeMapClear(&routes, -1);
        }

        PROFILER_END();
    }

    TownSortAVL *sorted = NULL, *top = NULL;
    int n = 0;

    {
        PROFILER_START("Sort towns");

        sortTowns(&stats, idCounter, &sorted);
        extractTop10(sorted, &top, &n);

        PROFILER_END();
    }
    printTop10(top);

    routeMapFree(&routes);
    townMapFree(&towns);
    partitionerFree(&partitioner);
    free(stats.elements);
    memFree(&townSortAVLMem);
    memFree(&townNodeListMem);
    memFree(&townStringsMem);

    PROFILER_END();
}

#endif
