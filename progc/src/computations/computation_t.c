#include "compile_settings.h"

#if !EXPERIMENTAL_ALGO

#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "profile.h"

// AVL with all the ids
typedef struct IdAVL
{
    AVL_HEADER(IdAVL)

    uint32_t id;
} IdAVL;

typedef struct TownAVL
{
    AVL_HEADER(TownAVL)
    int passed; // Number of times this town has been passed.
    int firstTown;
    IdAVL* routeIds;
    char name[]; // Flexible array members, contains the name of the town.
} TownAVL;

typedef struct TownSortAVL
{
    AVL_HEADER(TownSortAVL)

    TownAVL* value; // Pointer to TownAVL node.
} TownSortAVL;

static IdAVL* idAVLCreate(uint32_t* id)
{
    IdAVL* A = malloc(sizeof(IdAVL)); // Create the tree using malloc.
    assert(A);
    A->id = *id;
    AVL_INIT(A);
    return A;
}

static int idAVLCompare(IdAVL* tree, uint32_t* id)
{
    return tree->id - *id;
}

AVL_DECLARE_FUNCTIONS_STATIC(idAVL, IdAVL, const uint32_t,
                             (AVLCreateFunc) &idAVLCreate, (AVLCompareValueFunc) &idAVLCompare)

static TownAVL* townAVLCreate(const char* townName)
{
    int chars = strlen(townName) + 1;

    // Create the tree using malloc, and alloc enough space for the name string.
    TownAVL* tree = malloc(sizeof(TownAVL) + chars);
    assert(tree);

    AVL_INIT(tree);
    strcpy(tree->name, townName);
    tree->passed = 0;
    tree->firstTown = 0;
    tree->routeIds = NULL;

    return tree;
}

static int townAVLCompare(TownAVL* tree, const char* townName)
{
    return strcmp(tree->name, townName);
}

AVL_DECLARE_FUNCTIONS_STATIC(townAVL, TownAVL, const char,
                             (AVLCreateFunc) &townAVLCreate, (AVLCompareValueFunc) &townAVLCompare)

static TownSortAVL* townSortAvlCreate(TownAVL* townNode)
{
    TownSortAVL* tree = malloc(sizeof(TownSortAVL));
    assert(tree);

    tree->value = townNode;
    AVL_INIT(tree);

    return tree;
}

static int townSortAVLComparePassed(TownSortAVL* tree, TownAVL* town)
{
    int deltaPassed = tree->value->passed - town->passed;
    if (deltaPassed != 0)
    {
        return deltaPassed;
    }
    else
    {
        return strcmp(tree->value->name, town->name);
    }
}

static int townSortAVLCompareName(TownSortAVL* tree, TownAVL* town)
{
    return strcmp(tree->value->name, town->name);
}

static AVL_DECLARE_INSERT_FUNCTION(townSortAvlInsertPassed, TownSortAVL, TownAVL,
                                   (AVLCreateFunc) &townSortAvlCreate, (AVLCompareValueFunc) &townSortAVLComparePassed)

static AVL_DECLARE_INSERT_FUNCTION(townSortAvlInsertName, TownSortAVL, TownAVL,
                                   (AVLCreateFunc) &townSortAvlCreate, (AVLCompareValueFunc) &townSortAVLCompareName)

static void sortTownsByPasses(TownAVL* townNode, TownSortAVL** sorted)
{
    if (townNode == NULL)
    {
        return;
    }

    *sorted = townSortAvlInsertPassed(*sorted, townNode, NULL, NULL);
    sortTownsByPasses(townNode->left, sorted);
    sortTownsByPasses(townNode->right, sorted);
}

static void createTop10(TownSortAVL* topTowns, int* n, TownSortAVL** sorted)
{
    if (topTowns == NULL || *n >= 10)
    {
        return;
    }

    createTop10(topTowns->right, n, sorted);

    if (*n < 10)
    {
        *sorted = townSortAvlInsertName(*sorted, topTowns->value, NULL, NULL);
        *n += 1;
    }

    createTop10(topTowns->left, n, sorted);
}

static void printTowns(TownSortAVL* t)
{
    if (t->left != NULL)
        printTowns(t->left);
    printf("%s;%d;%d\n", t->value->name, t->value->passed, t->value->firstTown);
    if (t->right != NULL)
        printTowns(t->right);
}

static void insertTown(TownAVL** towns, const RouteStep* step, const char* townName, bool townA)
{
    TownAVL* townNode;
    *towns = townAVLInsert(*towns, townName, &townNode, NULL);

    bool seenId;
    townNode->routeIds = idAVLInsert(townNode->routeIds, &step->routeId, NULL, &seenId);

    if (!seenId)
    {
        townNode->passed++;
    }

    if (townA && step->stepId == 1)
    {
        townNode->firstTown++;
    }
}

static void freeAVLBasic(AVL* tree)
{
    if (tree == NULL)
    {
        return;
    }

    freeAVLBasic(tree->left);
    freeAVLBasic(tree->right);

    free(tree);
}

static void freeTownAVL(TownAVL* tree)
{
    if (tree == NULL)
    {
        return;
    }

    freeTownAVL(tree->left);
    freeTownAVL(tree->right);

    freeAVLBasic((AVL*) tree->routeIds);
    free(tree);
}

void computationT(RouteStream* stream)
{
    PROFILER_START("Computation T");

    TownAVL* towns = NULL;

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | STEP_ID | TOWN_A | TOWN_B))
    {
        insertTown(&towns, &step, step.townA, true);
        insertTown(&towns, &step, step.townB, false);
    }

    int n = 0;
    TownSortAVL* sorted = NULL;
    TownSortAVL* top10 = NULL;

    sortTownsByPasses(towns, &sorted);
    createTop10(sorted, &n, &top10);
    printTowns(top10);

    freeTownAVL(towns);
    freeAVLBasic((AVL*) sorted);
    freeAVLBasic((AVL*) top10);

    PROFILER_END();
}

#endif
