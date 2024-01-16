#include "compile_settings.h"

#if EXPERIMENTAL_ALGO

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "avl.h"
#include "computations.h"
#include "route.h"
#include "profile.h"
#include "map.h"
#include "mem_alloc.h"

MemArena travelSortAVLMem;

typedef struct TravelEntry
{
    bool occupied : 1;
    uint32_t id : 31;
    float min;
    float max;
    // In the first step, where we read the file, this will be the sum of all the distances.
    // Once we have finished this, in the calcAvg function, this will be the average distance.
    // This approach saves some memory instead of having two different fields (sum and avg).
    float sumOrAvg;
    uint32_t nSteps;
} TravelEntry;

typedef struct
{
    MAP_HEADER(TravelEntry)
} TravelMap;

#define CURRENT_MAP_TYPE() TravelMap

static inline uint32_t MAP_HASH_FUNC(const int* key, uint32_t capacityExponent)
{
    uint32_t a = * (uint32_t*) key;
    a *= 2654435769U;
    return a >> (32 - capacityExponent);
}

static inline bool MAP_KEY_EQUAL_FUNC(const TravelEntry* entry, const int* key)
{
    return entry->id == *key;
}

static inline bool MAP_GET_OCCUPIED_FUNC(const TravelEntry* entry)
{
    return entry->occupied;
}

static inline void MAP_MARK_OCCUPIED_FUNC(TravelEntry* entry, int* key)
{
    entry->occupied = true;
    entry->id = *key;
}

static inline uint32_t* MAP_GET_KEY_PTR_FUNC(TravelEntry* entry, uint64_t* scratch)
{
    uint32_t* scratch32 = (uint32_t*) scratch;

    *scratch32 = entry->id;
    return scratch32;
}

MAP_DECLARE_FUNCTIONS_STATIC(travelMap, TravelEntry, int, true)

#undef CURRENT_MAP_TYPE

// The AVL with all the travels sorted by their (max-min) value.
typedef struct TravelSortAVL
{
    AVL_HEADER(TravelSortAVL)

    float deltaMaxMin;
    TravelEntry* t; // Points to a travel in TravelAVL.
} TravelSortAVL;

TravelSortAVL* travelSortAVLCreate(TravelEntry* travel)
{
    TravelSortAVL* tree = memAlloc(&travelSortAVLMem, sizeof(TravelSortAVL));

    AVL_INIT(tree);
    tree->t = travel;
    tree->deltaMaxMin = travel->max - travel->min;

    return tree;
}

int travelSortAVLCompare(TravelSortAVL* tree, TravelEntry* travel)
{
    float deltaMaxMin = tree->deltaMaxMin - (travel->max - travel->min);
    if (deltaMaxMin < 0)
    {
        return -1;
    }
    else if (deltaMaxMin > 0)
    {
        return 1;
    }
    else
    {
        return tree->t->id - travel->id;
    }
}

AVL_DECLARE_INSERT_FUNCTION(travelSortAVLInsert, TravelSortAVL, TravelEntry,
                            (AVLCreateFunc) &travelSortAVLCreate, (AVLCompareValueFunc) &travelSortAVLCompare)

// Once we have accumulated all the distances, calculate all the average distances of the travels.
void calcAvgAndSort(TravelMap* map, TravelSortAVL** sorted)
{
    for (uint32_t i = 0; i < map->capacity; ++i)
    {
        TravelEntry* entry = &map->entries[i];
        if (entry->occupied)
        {
            // At this moment, avgOrSum contains the sum of all distances.
            // Transform it into an average.
            entry->sumOrAvg = entry->sumOrAvg / entry->nSteps;
        }

        *sorted = travelSortAVLInsert(*sorted, entry, NULL, NULL);
    }
}

void printTop50(TravelSortAVL* tr, int* n)
{
    if (tr == NULL || *n >= 50)
    {
        return;
    }

    printTop50(tr->right, n);

    if (*n < 50)
    {
        *n += 1;
        printf("%d;%d;%f;%f;%f;%f\n", *n,
               tr->t->id, tr->t->min, tr->t->sumOrAvg, tr->t->max, tr->t->max - tr->t->min);
    }

    printTop50(tr->left, n);
}

void computationS(RouteStream* stream)
{
    PROFILER_START("Computation S (Experimental!)");

    memInit(&travelSortAVLMem, 1 * 1024 * 1024);

    TravelMap travels;
    travelMapInit(&travels, 1 << 16, 0.7f);

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | DISTANCE))
    {
        TravelEntry* travel = travelMapLookup(&travels, step.routeId);
        if (travel == NULL)
        {
            travel = travelMapInsert(&travels, step.routeId);
            travel->max = step.distance;
            travel->min = step.distance;
            travel->sumOrAvg = step.distance; // Sum of all the distances.
            travel->nSteps = 1;
        }
        else
        {
            if (travel->max < step.distance)
            {
                travel->max = step.distance;
            }
            if (travel->min > step.distance)
            {
                travel->min = step.distance;
            }
            travel->sumOrAvg += step.distance; // Add to the sum of all distances.
            travel->nSteps += 1;
        }
    }

    TravelSortAVL* sorted = NULL;
    int n = 0;

    // Transform the sum into an average.
    calcAvgAndSort(&travels, &sorted);
    printTop50(sorted, &n);

    memFree(&travelSortAVLMem);

    PROFILER_END();
}

#endif