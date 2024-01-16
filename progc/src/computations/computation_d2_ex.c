#include "computations.h"
#include <stdlib.h>
#include <stdio.h>

#if EXPERIMENTAL_ALGO
#include <string.h>

#include "route.h"
#include "avl.h"
#include "profile.h"
#include "mem_alloc.h"

MemArena driverAVLMem;
MemArena driverSortAVLMem;

typedef struct DriverAVL
{
    AVL_HEADER(DriverAVL)

    float dist;
    char name[]; // Flexible array member
} DriverAVL;

static DriverAVL* driverAVLCreate(const char* name)
{
    size_t len = strlen(name);
    DriverAVL* tree = memAlloc(&driverAVLMem, sizeof(DriverAVL) + len + 1);
    // Stuff allocated by memAlloc do not need to be checked.

    AVL_INIT(tree);
    tree->dist = 0.0f;
    memcpy(tree->name, name, len + 1);

    return tree;
}

static int driverAVLCompare(DriverAVL* tree, const char* name)
{
    return strcmp(tree->name, name);
}

AVL_DECLARE_FUNCTIONS_STATIC(driverAVL, DriverAVL, const char,
                             (AVLCreateFunc) &driverAVLCreate, (AVLCompareValueFunc) &driverAVLCompare)

typedef struct DriverSortAVL
{
    AVL_HEADER(DriverSortAVL)

    DriverAVL* driver;
} DriverSortAVL;

static DriverSortAVL* driverSortAVLCreate(DriverAVL* driver)
{
    DriverSortAVL* tree = memAlloc(&driverSortAVLMem, sizeof(DriverSortAVL));

    AVL_INIT(tree);
    tree->driver = driver;

    return tree;
}

static int driverSortAVLCompare(DriverSortAVL* tree, DriverAVL* driver)
{
    if (tree->driver->dist > driver->dist)
    {
        return 1;
    }
    else if (tree->driver->dist < driver->dist)
    {
        return -1;
    }
    else
    {
        return strcmp(tree->driver->name, driver->name);
    }
}

AVL_DECLARE_FUNCTIONS_STATIC(driverSortAVL, DriverSortAVL, DriverAVL,
                             (AVLCreateFunc) &driverSortAVLCreate, (AVLCompareValueFunc) &driverSortAVLCompare)

static void sortDrivers(DriverAVL* drivers, DriverSortAVL** sorted)
{
    if (drivers == NULL)
    {
        return;
    }

    *sorted = driverSortAVLInsert(*sorted, drivers, NULL, NULL);
    sortDrivers(drivers->left, sorted);
    sortDrivers(drivers->right, sorted);
}

static void printTop10(DriverSortAVL* tree, int* n)
{
    if (tree == NULL || *n >= 10)
    {
        return;
    }

    printTop10(tree->right, n);

    if (*n < 10)
    {
        printf("%s;%f\n", tree->driver->name, tree->driver->dist);
        *n += 1;
    }

    printTop10(tree->left, n);
}

void computationD2(RouteStream* stream)
{
    PROFILER_START("Computation D2");

    DriverAVL* drivers = NULL;

    memInit(&driverAVLMem, 512 * 1024);
    memInit(&driverSortAVLMem, 128 * 1024);

    RouteStep step;
    while (rsRead(stream, &step, DRIVER_NAME | DISTANCE))
    {
        DriverAVL* driver = driverAVLLookup(drivers, step.driverName);
        if (driver == NULL)
        {
            drivers = driverAVLInsert(drivers, step.driverName, &driver, NULL);
        }

        driver->dist += step.distance;
    }

    DriverSortAVL* sorted = NULL;
    int n = 0;

    sortDrivers(drivers, &sorted);
    printTop10(sorted, &n);

    memFree(&driverAVLMem);
    memFree(&driverSortAVLMem);

    PROFILER_END();
}

#else
void computationD2(struct RouteStream* s)
{
    fprintf(stderr, "Cannot use computation D2 without experimental algorithms enabled!\n");
    exit(9);
}
#endif
