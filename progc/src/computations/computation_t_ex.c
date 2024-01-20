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

/*
 * [EXPERIMENTAL!] Computation T implementation
 * Featuring: The experimental map structure, and the memory arena allocator!
 */

// The memory arenas used for each kind of structure.
// Currently those are global variables, should these be passed to the create functions instead?
MemArena townAVLMem; // Used to allocate TownAVL nodes.
MemArena townSortAVLMem; // Used to allocate TownSortAVL nodes.
MemArena townNodeListMem; // Used to allocate the TownNodeList structures (only the ones we need to allocate).
MemArena townStringsMem;

// Can be changed to uint16_t for 2x more towns stored, but limits the total amount of towns to 65536.
typedef uint32_t TownNodeId;

#define TN_LIST_SIZE 132
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
    uint32_t passed;
    uint32_t firstTown;
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

typedef struct TownSortAVL
{
    AVL_HEADER(TownSortAVL)

    uint32_t passed;
    char* name;
    TownMapEntry* entry;
} TownSortAVL;

TownSortAVL* townSortAVLCreate(TownMapEntry* entry)
{
    TownSortAVL* tree = memAlloc(&townSortAVLMem, sizeof(TownSortAVL));
    // assert(tree);

    AVL_INIT(tree);
    tree->entry = entry;
    tree->name = entry->name;
    tree->passed = entry->passed;

    return tree;
}

int townSortAVLComparePassed(TownSortAVL* tree, TownMapEntry* entry)
{
    int cmp = tree->passed - entry->passed;
    if (cmp != 0)
    {
        return cmp;
    }
    else
    {
        return strcmp(tree->name, entry->name);
    }
}

int townSortAVLCompareName(TownSortAVL* tree, TownMapEntry* townNode)
{
    return strcmp(tree->name, townNode->name);
}

AVL_DECLARE_INSERT_FUNCTION(townSortAVLInsertPassed, TownSortAVL, TownMapEntry,
                            (AVLCreateFunc) &townSortAVLCreate, (AVLCompareValueFunc) &townSortAVLComparePassed)

AVL_DECLARE_INSERT_FUNCTION(townSortAVLInsertName, TownSortAVL, TownMapEntry,
                            (AVLCreateFunc) &townSortAVLCreate, (AVLCompareValueFunc) &townSortAVLCompareName)

static inline void insertTown(const RouteStep* step, RouteEntry* routeNode, TownMap* towns, MeasuredString townName,
                              bool isTownA, TownNodeId* idCounter)
{
    TownMapEntry* townNode = townMapLookup(towns, townName);
    if (townNode == NULL)
    {
        townNode = townMapInsert(towns, townName);
        townNode->id = (*idCounter)++;
    }

    if (isTownA && step->stepId == 1)
    {
        townNode->firstTown++;
    }

    if (!tnListSearch(&routeNode->towns, townNode->id))
    {
        tnListAdd(&routeNode->towns, townNode->id);
        townNode->passed++;
    }
}

void sortTowns(TownMap* towns, TownSortAVL** sorted)
{
    for (uint32_t i = 0; i < towns->capacity; ++i)
    {
        TownMapEntry* entry = &towns->entries[i];
        if (entry->occupied)
        {
            *sorted = townSortAVLInsertPassed(*sorted, entry, NULL, NULL);
        }
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
        *top = townSortAVLInsertName(*top, sorted->entry, NULL, NULL);
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
    printf("%s;%d;%d\n", top->name, top->passed, top->entry->firstTown);
    printTop10(top->right);
}

void computationT(RouteStream* stream)
{
    PROFILER_START("Computation T (Experimental!)");

    memInit(&townAVLMem, 1 * 1024 * 1024);
    memInit(&townSortAVLMem, 256 * 1024);
    memInit(&townNodeListMem, 1 * 1024 * 1024);
    memInitEx(&townStringsMem, 512 * 1024, 1);

    RouteMap routes;
    TownMap towns;
    TownNodeId idCounter = 0;

    routeMapInit(&routes, 1 << 16, 0.75f);
    townMapInit(&towns, 8192, 0.75f);

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | STEP_ID | TOWN_A | TOWN_B))
    {
        RouteEntry* entry = routeMapLookup(&routes, step.routeId);
        if (entry == NULL)
        {
            entry = routeMapInsert(&routes, step.routeId);
            tnListInit(&entry->towns);
        }

        MeasuredString townA = { step.townA, step.townALen };
        MeasuredString townB = { step.townB, step.townBLen };
        insertTown(&step, entry, &towns, townA, true, &idCounter);
        insertTown(&step, entry, &towns, townB, false, &idCounter);
    }

    TownSortAVL *sorted = NULL, *top = NULL;
    int n = 0;

    sortTowns(&towns, &sorted);
    extractTop10(sorted, &top, &n);
    printTop10(top);

    townMapFree(&towns);
    memFree(&townAVLMem);
    memFree(&townSortAVLMem);
    memFree(&townNodeListMem);
    memFree(&townStringsMem);

    PROFILER_END();
}

#endif
