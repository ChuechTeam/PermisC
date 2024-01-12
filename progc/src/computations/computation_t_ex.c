#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "string_avl.h"
#include <stdalign.h>

#include "profile.h"
#include "compile_settings.h"

#if EXPERIMENTAL_ALGO

typedef uint32_t TownNodeId;

const TownNodeId NULL_TNID = UINT32_MAX;

#define TN_LIST_SIZE 128
#define TN_LIST_NUM TN_LIST_SIZE/sizeof(TownNodeId)

typedef struct TownNodeList
{
    uint16_t size;
    struct TownNodeList* next;
    alignas(8) TownNodeId nodes[TN_LIST_NUM];
} TownNodeList;

void tnListInit(TownNodeList* list)
{
    memset(list->nodes, 0xFF, TN_LIST_SIZE);
    list->size = 0;
    list->next = NULL;
}

TownNodeList* tnListCreate()
{
    TownNodeList* list = malloc(sizeof(TownNodeList));
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
    TownNodeList towns;
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
    RouteAVL* tree = malloc(sizeof(RouteAVL));
    assert(tree);

    AVL_INIT(tree);
    tree->routeId = *routeId;
    tnListInit(&tree->towns);

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
    TownAVL* tree = malloc(sizeof(TownAVL) + len + 1);

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
    TownSortAVL* tree = malloc(sizeof(TownSortAVL));
    assert(tree);

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

static void insertTown(const RouteStep* step, RouteAVL* routeNode, TownAVL** towns, const char* townName, bool isTownA,
                       TownNodeId* idCounter)
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

    if (!tnListSearch(&routeNode->towns, townNode->id))
    {
        tnListAdd(&routeNode->towns, townNode->id);
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

    PROFILER_END();
}

#endif
