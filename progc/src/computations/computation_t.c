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

    int id;
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

IdAVL* idAVLCreate(int* id)
{
    IdAVL* A = malloc(sizeof(IdAVL)); // création de l'AVL d'ID
    assert(A);
    A->id = *id;
    AVL_INIT(A);
    return A;
}

int idAVLCompare(IdAVL* tree, int* id)
{
    return tree->id - *id;
}

AVL_DECLARE_INSERT_FUNCTION(idAVLInsert, IdAVL, const int,
                            (AVLCreateFunc) &idAVLCreate, (AVLCompareValueFunc) &idAVLCompare)
AVL_DECLARE_LOOKUP_FUNCTION(idAVLLookup, IdAVL, const int,
                            (AVLCompareValueFunc) &idAVLCompare)

TownAVL* townAVLCreate(const char* townName)
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

int townAVLCompare(TownAVL* tree, const char* townName)
{
    return strcmp(tree->name, townName);
}

AVL_DECLARE_INSERT_FUNCTION(townAVLInsert, TownAVL, const char,
                            (AVLCreateFunc) &townAVLCreate, (AVLCompareValueFunc) &townAVLCompare)
AVL_DECLARE_LOOKUP_FUNCTION(townAVLLookup, TownAVL, const char,
                            (AVLCompareValueFunc) &townAVLCompare)

TownSortAVL* townSortAvlCreate(TownAVL* townNode)
{
    TownSortAVL* tree = malloc(sizeof(TownSortAVL));
    assert(tree);

    tree->value = townNode;
    AVL_INIT(tree);

    return tree;
}

int townSortAVLComparePassed(TownSortAVL* A, TownAVL* T)
{
    int deltaPassed = A->value->passed - T->passed;
    if (deltaPassed != 0)
    {
        return deltaPassed;
    }
    else
    {
        return strcmp(A->value->name, T->name);
    }
}

int townSortAVLCompareName(TownSortAVL* A, TownAVL* T)
{
    return strcmp(A->value->name, T->name);
}

AVL_DECLARE_INSERT_FUNCTION(townSortAvlInsertPassed, TownSortAVL, TownAVL,
                            (AVLCreateFunc) &townSortAvlCreate, (AVLCompareValueFunc) &townSortAVLComparePassed)
AVL_DECLARE_INSERT_FUNCTION(townSortAvlInsertName, TownSortAVL, TownAVL,
                            (AVLCreateFunc) &townSortAvlCreate, (AVLCompareValueFunc) &townSortAVLCompareName)

void sortTownsByPasses(TownAVL* townNode, TownSortAVL** sorted)
{
    if (townNode == NULL)
    {
        return;
    }

    *sorted = townSortAvlInsertPassed(*sorted, townNode, NULL, NULL);
    sortTownsByPasses(townNode->left, sorted);
    sortTownsByPasses(townNode->right, sorted);
}

void createTop10(TownSortAVL* topTowns, int* n, TownSortAVL** sorted)
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

void printTowns(TownSortAVL* C)
{
    if (C->left != NULL)
        printTowns(C->left);
    printf("%s;%d;%d\n", C->value->name, C->value->passed, C->value->firstTown);
    if (C->right != NULL)
        printTowns(C->right);
}

void insertTown(TownAVL** towns, const RouteStep* step, const char* townName, bool townA)
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

void freeAVLBasic(AVL* tree)
{
    if (tree == NULL)
    {
        return;
    }

    free(tree->left);
    free(tree->right);

    free(tree);
}

void freeTownAVL(TownAVL* tree)
{
    if (tree == NULL)
    {
        return;
    }

    free(tree->left);
    free(tree->right);

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