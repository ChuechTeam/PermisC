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

/*
 * [EXPERIMENTAL!] Computation T implementation
 */

// The memory arenas used for each kind of structure.
// Currently those are global variables, should these be passed to the create functions instead?
MemArena routeAVLMem; // Used to allocate RouteAVL nodes.
MemArena townAVLMem; // Used to allocate TownAVL nodes.
MemArena townSortAVLMem; // Used to allocate TownSortAVL nodes.
MemArena townNodeListMem; // Used to allocate the TownNodeList structures.

typedef uint32_t TownNodeId;

#define TN_LIST_SIZE 132
#define TN_LIST_NUM TN_LIST_SIZE/sizeof(TownNodeId)

typedef struct TownNodeList
{
    struct TownNodeList* next;
    uint16_t size;
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
    for (int i = 0; i < list->size; ++i)
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

typedef struct RouteAVL
{
    AVL_HEADER(RouteAVL)

    int routeId;
    // Allocated in the memory arena.
    TownNodeList* towns;
} RouteAVL;

typedef struct TownAVL
{
    AVL_HEADER(TownAVL)

    TownNodeId id;
    uint32_t passed; // Number of times this town has been passed.
    uint32_t firstTown;
    char name[]; // Flexible array members, contains the name of the town.
} TownAVL;

typedef struct TownSortAVL
{
    AVL_HEADER(TownSortAVL)

    TownAVL* townNode;
} TownSortAVL;

static RouteAVL* routeAVLCreate(const int* routeId)
{
    RouteAVL* tree = memAlloc(&routeAVLMem, sizeof(RouteAVL));

    AVL_INIT(tree);
    tree->routeId = *routeId;
    tree->towns = memAlloc(&townNodeListMem, sizeof(TownNodeList));
    tnListInit(tree->towns);

    return tree;
}

static int routeAVLCompare(const RouteAVL* tree, const int* routeId)
{
    return tree->routeId - *routeId;
}

AVL_DECLARE_FUNCTIONS_STATIC(routeAVL, RouteAVL, const int,
                             (AVLCreateFunc) &routeAVLCreate, (AVLCompareValueFunc) &routeAVLCompare)

static TownAVL* townAVLCreate(const char* townName)
{
    size_t len = strlen(townName);
    TownAVL* tree = memAlloc(&townAVLMem, sizeof(TownAVL) + len + 1);

    AVL_INIT(tree);
    tree->passed = 0;
    tree->firstTown = 0;
    memcpy(tree->name, townName, len + 1);

    return tree;
}

static int townAVLCompare(const TownAVL* tree, const char* townName)
{
    return strcmp(tree->name, townName);
}

AVL_DECLARE_FUNCTIONS_STATIC(townAVL, TownAVL, const char,
                             (AVLCreateFunc) &townAVLCreate, (AVLCompareValueFunc) &townAVLCompare)

TownSortAVL* townSortAVLCreate(TownAVL* townNode)
{
    TownSortAVL* tree = memAlloc(&townSortAVLMem, sizeof(TownSortAVL));
    // assert(tree);

    AVL_INIT(tree);
    tree->townNode = townNode;

    return tree;
}

int townSortAVLComparePassed(TownSortAVL* tree, TownAVL* townNode)
{
    int cmp = tree->townNode->passed - townNode->passed;
    if (cmp != 0)
    {
        return cmp;
    }
    else
    {
        return strcmp(tree->townNode->name, townNode->name);
    }
}

int townSortAVLCompareName(TownSortAVL* tree, TownAVL* townNode)
{
    return strcmp(tree->townNode->name, townNode->name);
}

AVL_DECLARE_INSERT_FUNCTION(townSortAVLInsertPassed, TownSortAVL, TownAVL,
                            (AVLCreateFunc) &townSortAVLCreate, (AVLCompareValueFunc) &townSortAVLComparePassed)

AVL_DECLARE_INSERT_FUNCTION(townSortAVLInsertName, TownSortAVL, TownAVL,
                            (AVLCreateFunc) &townSortAVLCreate, (AVLCompareValueFunc) &townSortAVLCompareName)

static inline void insertTown(const RouteStep* step, RouteAVL* routeNode, TownAVL** towns, const char* townName,
                              bool isTownA, TownNodeId* idCounter)
{
    TownAVL* townNode = townAVLLookup(*towns, townName);
    if (townNode == NULL)
    {
        *towns = townAVLInsert(*towns, townName, &townNode, NULL);
        townNode->id = (*idCounter)++;
    }

    if (isTownA && step->stepId == 1)
    {
        townNode->firstTown++;
    }

    if (!tnListSearch(routeNode->towns, townNode->id))
    {
        tnListAdd(routeNode->towns, townNode->id);
        townNode->passed++;
    }
}

void sortTowns(TownAVL* townNode, TownSortAVL** sorted)
{
    if (townNode == NULL)
    {
        return;
    }

    *sorted = townSortAVLInsertPassed(*sorted, townNode, NULL, NULL);
    sortTowns(townNode->left, sorted);
    sortTowns(townNode->right, sorted);
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
        *top = townSortAVLInsertName(*top, sorted->townNode, NULL, NULL);
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
    printf("%s;%d;%d\n", top->townNode->name, top->townNode->passed, top->townNode->firstTown);
    printTop10(top->right);
}

void computationT(RouteStream* stream)
{
    PROFILER_START("Computation T (Experimental!)");

    RouteAVL* routes = NULL;
    TownAVL* towns = NULL;
    TownNodeId idCounter = 0;

    memInit(&routeAVLMem, 1 * 1024 * 1024);
    memInit(&townAVLMem, 1 * 1024 * 1024);
    memInit(&townSortAVLMem, 256 * 1024);
    memInit(&townNodeListMem, 1 * 1024 * 1024);

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | STEP_ID | TOWN_A | TOWN_B))
    {
        RouteAVL* routeNode = routeAVLLookup(routes, &step.routeId);
        if (routeNode == NULL)
        {
            routes = routeAVLInsert(routes, &step.routeId, &routeNode, NULL);
        }

        insertTown(&step, routeNode, &towns, step.townA, true, &idCounter);
        insertTown(&step, routeNode, &towns, step.townB, false, &idCounter);
    }

    TownSortAVL *sorted = NULL, *top = NULL;
    int n = 0;

    sortTowns(towns, &sorted);
    extractTop10(sorted, &top, &n);
    printTop10(top);

    memFree(&routeAVLMem);
    memFree(&townAVLMem);
    memFree(&townSortAVLMem);
    memFree(&townNodeListMem);

    PROFILER_END();
}

#endif
