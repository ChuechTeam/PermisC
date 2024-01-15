#include "compile_settings.h"

#if !EXPERIMENTAL_ALGO

#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "profile.h"

typedef struct Travel
{
    uint32_t id;
    float min;
    float max;
    // In the first step, where we read the file, this will be the sum of all the distances.
    // Once we have finished this, in the calcAvg function, this will be the average distance.
    // This approach saves some memory instead of having two different fields (sum and avg).
    float sumOrAvg;
    uint32_t nSteps;
} Travel;

// The AVL containing all the travels, before we sort them.
typedef struct TravelAVL
{
    AVL_HEADER(TravelAVL)

    Travel t;
} TravelAVL;

// The AVL with all the travels sorted by their (max-min) value.
typedef struct TravelSortAVL
{
    AVL_HEADER(TravelSortAVL)

    Travel* t; // Points to a travel in TravelAVL.
} TravelSortAVL;

TravelAVL* travelAVLCreate(Travel* travel)
{
    TravelAVL* tree = malloc(sizeof(TravelAVL));
    assert(tree);

    tree->t = *travel; // Copy the travel.
    AVL_INIT(tree);

    return tree;
}

int travelAVLCompare(TravelAVL* tree, Travel* travel)
{
    return tree->t.id - travel->id;
}

AVL_DECLARE_INSERT_FUNCTION(travelAVLInsert, TravelAVL, Travel,
                            (AVLCreateFunc) &travelAVLCreate, (AVLCompareValueFunc) &travelAVLCompare)

AVL_DECLARE_LOOKUP_FUNCTION(travelAVLLookup, TravelAVL, Travel, (AVLCompareValueFunc) &travelAVLCompare)

// Once we have accumulated all the distances, calculate all the average distances of the travels.
void calcAvg(TravelAVL* tree)
{
    if (tree == NULL)
    {
        return;
    }

    // At this moment, avgOrSum contains the sum of all distances.
    // Transform it into an average.
    tree->t.sumOrAvg = tree->t.sumOrAvg / tree->t.nSteps;

    calcAvg(tree->left);
    calcAvg(tree->right);
}

TravelSortAVL* travelSortAVLCreate(Travel *travel) {
    TravelSortAVL *tree = malloc(sizeof(TravelSortAVL));
    assert(tree);

    tree->t = travel;
    AVL_INIT(tree);

    return tree;
}

int travelSortAVLCompare(TravelSortAVL* tree, Travel* travel)
{
    float deltaMaxMin = (tree->t->max - tree->t->min) - (travel->max - travel->min);
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

AVL_DECLARE_INSERT_FUNCTION(travelSortAVLInsert, TravelSortAVL, Travel,
                            (AVLCreateFunc) &travelSortAVLCreate, (AVLCompareValueFunc) &travelSortAVLCompare)

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

void transferToSortAVL(TravelAVL* travels, TravelSortAVL** sorted)
{
    if (travels == NULL)
    {
        return;
    }

    *sorted = travelSortAVLInsert(*sorted, &travels->t, NULL, NULL);

    transferToSortAVL(travels->left, sorted);
    transferToSortAVL(travels->right, sorted);
}

void freeAVL(AVL* tree)
{
    if (tree->left != NULL)
    {
        freeAVL(tree->left);
    }
    if (tree->right != NULL)
    {
        freeAVL(tree->right);
    }
    free(tree);
}

void computationS(RouteStream* stream)
{
    PROFILER_START("Computation S");

    TravelAVL* travels = NULL;

    RouteStep step;
    while (rsRead(stream, &step, ROUTE_ID | DISTANCE))
    {
        Travel tra = {.id = step.routeId};
        TravelAVL* found = travelAVLLookup(travels, &tra);
        if (found == NULL)
        {
            tra.max = step.distance;
            tra.min = step.distance;
            tra.sumOrAvg = step.distance; // Sum of all the distances.
            tra.nSteps = 1;

            travels = travelAVLInsert(travels, &tra, NULL, NULL);
        }
        else
        {
            if (found->t.max < step.distance)
            {
                found->t.max = step.distance;
            }
            if (found->t.min > step.distance)
            {
                found->t.min = step.distance;
            }
            found->t.sumOrAvg += step.distance; // Add to the sum of all distances.
            found->t.nSteps += 1;
        }
    }

    TravelSortAVL* sorted = NULL;
    int n = 0;

    // Transform the sum into an average.
    calcAvg(travels);
    transferToSortAVL(travels, &sorted);
    printTop50(sorted, &n);

    freeAVL((AVL*) sorted);
    freeAVL((AVL*) travels);

    PROFILER_END();
}

#endif