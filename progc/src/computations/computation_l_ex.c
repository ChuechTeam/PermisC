#include "computations.h"
#include <stdlib.h>
#include <stdio.h>

#if EXPERIMENTAL_ALGO
#include "route.h"
#include "avl.h"
#include "map.h"
#include "profile.h"
#include "mem_alloc.h"

MemArena routeSortAVLMem;

typedef struct RouteDistEntry
{
    bool occupied : 1;
    uint32_t id : 31;
    float dist;
} RouteDistEntry;

typedef struct
{
    MAP_HEADER(RouteDistEntry)
} RouteDistMap;

#define CURRENT_MAP_TYPE() RouteDistMap

static inline uint32_t MAP_HASH_FUNC(const int* key, uint32_t capacityExponent)
{
    uint32_t a = * (uint32_t*) key;
    a *= 2654435769U;
    return a >> (32 - capacityExponent);
}

static inline bool MAP_KEY_EQUAL_FUNC(const RouteDistEntry* entry, const int* key)
{
    return entry->id == *key;
}

static inline bool MAP_GET_OCCUPIED_FUNC(const RouteDistEntry* entry)
{
    return entry->occupied;
}

static inline void MAP_MARK_OCCUPIED_FUNC(RouteDistEntry* entry, int* key)
{
    entry->occupied = true;
    entry->id = *key;
}

static inline uint32_t* MAP_GET_KEY_PTR_FUNC(RouteDistEntry* entry, uint64_t* scratch)
{
    uint32_t* scratch32 = (uint32_t*) scratch;

    *scratch32 = entry->id;
    return scratch32;
}

MAP_DECLARE_FUNCTIONS_STATIC(routeDist, RouteDistEntry, int, true)

#undef CURRENT_MAP_TYPE

typedef struct { int routeId; float dist; } RouteSortInfo;

typedef struct RouteSortAVL
{
    AVL_HEADER(RouteSortAVL)

    RouteSortInfo info;
} RouteSortAVL;

static RouteSortAVL* routeSortAVLCreate(RouteSortInfo* info)
{
    RouteSortAVL* tree = memAlloc(&routeSortAVLMem, sizeof(RouteSortAVL));
    assert(tree);

    AVL_INIT(tree);
    tree->info = *info;

    return tree;
}

static int routeSortAVLCompareDist(RouteSortAVL* tree, RouteSortInfo* info)
{
    if (tree->info.dist > info->dist)
    {
        return 1;
    }
    else if (tree->info.dist < info->dist)
    {
        return -1;
    }
    else
    {
        return tree->info.routeId - info->routeId;
    }
}

static int routeSortAVLCompareId(RouteSortAVL* tree, RouteSortInfo* info)
{
    return tree->info.routeId - info->routeId;
}

static AVL_DECLARE_INSERT_FUNCTION(routeSortAVLInsertDist, RouteSortAVL, RouteSortInfo,
                            (AVLCreateFunc) &routeSortAVLCreate, (AVLCompareValueFunc) &routeSortAVLCompareDist)

static AVL_DECLARE_INSERT_FUNCTION(routeSortAVLInsertId, RouteSortAVL, RouteSortInfo,
                            (AVLCreateFunc) &routeSortAVLCreate, (AVLCompareValueFunc) &routeSortAVLCompareId)

static void extractTop10(RouteSortAVL* distSorted, RouteSortAVL** top, int* n)
{
    if (distSorted == NULL || *n >= 10)
    {
        return;
    }

    extractTop10(distSorted->right, top, n);

    if (*n < 10)
    {
        *top = routeSortAVLInsertId(*top, &distSorted->info, NULL, NULL);
        *n += 1;
    }

    extractTop10(distSorted->left, top, n);
}

static void printTop10(RouteSortAVL* top)
{
    if (top == NULL)
    {
        return;
    }

    printTop10(top->left);

    printf("%d;%f\n", top->info.routeId, top->info.dist);

    printTop10(top->right);
}

void computationL(RouteStream* stream)
{
    PROFILER_START("Computation L");

    memInit(&routeSortAVLMem, 1 * 1024 * 1024);

    RouteDistMap map;
    routeDistInit(&map, 1 << 16, 0.7f); // 65536 capacity

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | DISTANCE))
    {
        RouteDistEntry* entry = routeDistLookup(&map, step.routeId);
        if (entry == NULL)
        {
            entry = routeDistInsert(&map, step.routeId);
            entry->dist = 0.0f;
        }

        entry->dist += step.distance;
    }

    RouteSortAVL* distSorted = NULL;
    for (int i = 0; i < map.capacity; ++i)
    {
        if (map.entries[i].occupied)
        {
            RouteSortInfo info = {map.entries[i].id, map.entries[i].dist};
            distSorted = routeSortAVLInsertDist(distSorted, &info, NULL, NULL);
        }
    }

    RouteSortAVL* top = NULL;
    int n = 0;
    extractTop10(distSorted, &top, &n);
    printTop10(top);

    routeDistFree(&map);
    memFree(&routeSortAVLMem);

    PROFILER_END();
}

#else
void computationL(struct RouteStream*)
{
    fprintf(stderr, "Cannot use computation L without experimental algorithms enabled!\n");
    exit(9);
}
#endif