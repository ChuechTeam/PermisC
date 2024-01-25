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
#include "partition.h"

static MemArena travelSortAVLMem;

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
    uint32_t a = *(uint32_t*) key;
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

static inline uint32_t* MAP_GET_KEY_PTR_FUNC(TravelEntry* entry, MapKeyScratch scratch)
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
    uint32_t id;
    float min;
    float max;
    float avg; // Copies the travel entry from the map.
} TravelSortAVL;

static TravelSortAVL* travelSortAVLCreate(TravelEntry* travel)
{
    TravelSortAVL* tree = memAlloc(&travelSortAVLMem, sizeof(TravelSortAVL));

    AVL_INIT(tree);
    tree->id = travel->id;
    tree->min = travel->min;
    tree->max = travel->max;
    tree->avg = travel->sumOrAvg;
    tree->deltaMaxMin = travel->max - travel->min;

    return tree;
}

static int travelSortAVLCompare(TravelSortAVL* tree, TravelEntry* travel)
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
        return tree->id - travel->id;
    }
}

static AVL_DECLARE_INSERT_FUNCTION(travelSortAVLInsert, TravelSortAVL, TravelEntry,
                                   (AVLCreateFunc) &travelSortAVLCreate, (AVLCompareValueFunc) &travelSortAVLCompare)

// Find the element with the 50th highest max-min value.
// This will be used as a threshold to avoid inserting useless elements in the AVL tree
static float findThresholdSortAVL(TravelSortAVL* tree, float top[50], int* i)
{
    if (tree == NULL || *i >= 50)
    {
        return 0.0f;
    }

    findThresholdSortAVL(tree->right, top, i);
    if (*i < 50)
    {
        top[(*i)++] = tree->deltaMaxMin;
    }
    findThresholdSortAVL(tree->left, top, i);

    return top[49];
}

// Once we have accumulated all the distances, calculate all the average distances of the travels.
static void calcAvgAndSort(TravelMap* map, TravelSortAVL** sorted)
{
    uint32_t num = 0;
    float threshold = -1e18f;

    for (uint32_t i = 0; i < map->capacity; ++i)
    {
        TravelEntry* entry = &map->entries[i];

        // If an entry isn't occupied, max = min = 0
        // so we have high chances that this condition fails first.
        if ((entry->max - entry->min) >= threshold && entry->occupied)
        {
            // At this moment, avgOrSum contains the sum of all distances.
            // Transform it into an average.
            entry->sumOrAvg = entry->sumOrAvg / entry->nSteps;
            *sorted = travelSortAVLInsert(*sorted, entry, NULL, NULL);

            num++;
            if (num >= 50)
            {
                float top[50];
                int ti = 0;
                threshold = findThresholdSortAVL(*sorted, top, &ti);
            }
        }
    }
}

static void printTop50(TravelSortAVL* tr, int* n)
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
               tr->id, tr->min, tr->avg, tr->max, tr->max - tr->min);
    }

    printTop50(tr->left, n);
}

typedef struct RoutePart
{
    uint32_t id;
    float dist;
} RoutePart;

void computationS(RouteStream* stream)
{
    PROFILER_START("Computation S (Experimental!)");

    memInit(&travelSortAVLMem, 1 * 1024 * 1024);

    TravelMap travels;
    travelMapInit(&travels, 1024, 0.7f);

    Partitioner partitioner;
    partitionerInit(&partitioner, 64, sizeof(RoutePart) * 10000);

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | DISTANCE))
    {
        RoutePart item = {step.routeId, step.distance};
        partinitionerAddS(&partitioner, step.routeId, item);
    }

    PARTITIONER_ITERATE(&partitioner, RoutePart, stepPart)
    {
        TravelEntry* travel = travelMapLookup(&travels, stepPart->id);
        if (travel == NULL)
        {
            travel = travelMapInsert(&travels, stepPart->id);
            travel->max = stepPart->dist;
            travel->min = stepPart->dist;
            travel->sumOrAvg = stepPart->dist; // Sum of all the distances
            travel->nSteps = 1;
        }
        else
        {
            if (travel->max < stepPart->dist)
            {
                travel->max = stepPart->dist;
            }
            if (travel->min > stepPart->dist)
            {
                travel->min = stepPart->dist;
            }
            travel->sumOrAvg += stepPart->dist; // Add to the sum of all distances.
            travel->nSteps += 1;
        }
    }

    TravelSortAVL* sorted = NULL;
    int n = 0;

    calcAvgAndSort(&travels, &sorted);
    printTop50(sorted, &n);

    travelMapFree(&travels);
    partitionerFree(&partitioner);
    memFree(&travelSortAVLMem);

    PROFILER_END();
}

#endif
