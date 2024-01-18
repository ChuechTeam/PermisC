#include <stdlib.h>
#include <stdio.h>
#include "compile_settings.h"

#if EXPERIMENTAL_ALGO

#include <assert.h>
#include <string.h>

#include "computations.h"
#include "route.h"
#include "string_avl.h"
#include "profile.h"
#include "map.h"
#include "mem_alloc.h"

static MemArena driverStringListMem;
static MemArena driverAVLMem;
static MemArena driverSortAVLMem;

// Computation D1
// ------------------------
// We need to find the 10 drivers who have driven the most routes.
// Routes can be driven by multiple drivers, so we need to avoid duplicates with a [routeId, driverName] pair.

/*
 * ------------
 * LINKED LIST OF STRINGS
 * ------------
 */

// A linked list of strings.
// The list is considered empty when its value is NULL.
typedef struct LLStr
{
    // The string contained in this linked list node.
    // When NULL, it indicates that this node is the head of the list, with zero elements.
    char* value;
    // The next element in the list, allocated using malloc.
    struct LLStr* next;
} LLStr;

// Adds a string to the end of the list. The list pointer must not be NULL.
static void llStrAdd(LLStr* list, char* value)
{
    assert(list);

    if (list->value == NULL)
    {
        list->value = value;
    }
    else
    {
        // Go to the end of the list.
        while (list->next != NULL) { list = list->next; }

        LLStr* newNode = memAlloc(&driverStringListMem, sizeof(LLStr));
        assert(newNode);

        newNode->value = value;
        newNode->next = NULL;
        list->next = newNode;
    }
}

/*
 * ------------
 * [Experimental!] ROUTE MAP: A hash map of routes to their drivers.
 * ------------
 */

// Key: RouteId
// Value: Linked list of drivers who have driven this route.
typedef struct RouteMapEntry
{
    bool occupied : 1;
    uint32_t key : 31;
    LLStr drivers;
} RouteMapEntry;

// This route map works using open addressing.
typedef struct
{
    MAP_HEADER(RouteMapEntry)
} RouteMap;

#define CURRENT_MAP_TYPE() RouteMap

static inline uint32_t MAP_HASH_FUNC(const void* key, uint32_t capacityExponent)
{
    uint32_t a = * (uint32_t*) key;
    a *= 2654435769U;
    return a >> (32 - capacityExponent);
}

static inline bool MAP_KEY_EQUAL_FUNC(const RouteMapEntry* entry, const uint32_t* key)
{
    return entry->key == *key;
}

static inline bool MAP_GET_OCCUPIED_FUNC(const RouteMapEntry* entry)
{
    return entry->occupied;
}

static inline void MAP_MARK_OCCUPIED_FUNC(RouteMapEntry* entry, uint32_t* key)
{
    entry->occupied = true;
    entry->key = *key;
}

static inline uint32_t* MAP_GET_KEY_PTR_FUNC(RouteMapEntry* entry, uint64_t* scratch)
{
    uint32_t* scratch32 = (uint32_t*) scratch;

    *scratch32 = entry->key;
    return scratch32;
}

MAP_DECLARE_FUNCTIONS_STATIC(routeMap, RouteMapEntry, uint32_t, true)

#undef CURRENT_MAP_TYPE

/*
 * DRIVER AVL
 */

typedef struct DriverAVL
{
    AVL_HEADER(DriverAVL)

    uint32_t routeCount;
    char name[];
} DriverAVL;

static DriverAVL* driverAVLCreate(const char* name)
{
    size_t len = strlen(name);
    DriverAVL* node = memAlloc(&driverAVLMem, sizeof(DriverAVL) + len + 1);
    assert(node);

    AVL_INIT(node);
    memcpy(node->name, name, len + 1);
    node->routeCount = 0;

    return node;
}

static int driverAVLCompare(const DriverAVL* a, const char* name)
{
    return strcmp(a->name, name);
}

AVL_DECLARE_FUNCTIONS_STATIC(driverAVL, DriverAVL, const char,
                             (AVLCreateFunc) &driverAVLCreate, (AVLCompareValueFunc) &driverAVLCompare)

/*
 * ------------
 * DRIVER SORT AVL
 * ------------
 */

// The AVL used to sort drivers by the number of routes taken.
// The ordering for AVL nodes uses the routes taken first, driver name second.
// For example:
//    [1, "A"] < [2, "A"] < [2, "B"] < [3, "A"]
// In AVL form, this becomes:
//        [2, "A"]          |
//       /        \         |
//  [1, "A"]   [3, "A"]     |
//             /            |
//       [2, "B"]           |
typedef struct DriverSortAVL
{
    AVL_HEADER(DriverSortAVL)

    // The number of routes taken, the first criteria for sorting.
    int routesTaken;

    // The name of the driver, the second criteria for sorting.
    // In our computation, it is allocated in the drivers AVL.
    char* driverName;
} DriverSortAVL;

// The struct containing all the data needed to insert a new DriverSortAVL node.
typedef struct
{
    int routesTaken;
    char* driverName;
} DriverSortAVLData;

static DriverSortAVL* driverSortAVLCreate(const DriverSortAVLData* data)
{
    DriverSortAVL* avl = memAlloc(&driverSortAVLMem, sizeof(DriverSortAVL));
    // No need to assert memAlloc.

    AVL_INIT(avl); // Initializes left, right and balance.
    avl->routesTaken = data->routesTaken;
    avl->driverName = data->driverName;

    return avl;
}

// First compare the routes taken, then the driver name.
static int driverSortAVLCompare(const DriverSortAVL* a, const DriverSortAVLData* data)
{
    int diff = a->routesTaken - data->routesTaken;
    if (diff != 0)
    {
        return diff;
    }
    else
    {
        return strcmp(a->driverName, data->driverName);
    }
}

static AVL_DECLARE_INSERT_FUNCTION(driverSortAVLInsert, DriverSortAVL, DriverSortAVLData,
                            (AVLCreateFunc) &driverSortAVLCreate, (AVLCompareValueFunc) &driverSortAVLCompare)

/*
 * ------------
 * THE COMPUTATION (at last!)
 * ------------
 */

// Define the prototype of the functions used in computationD1 first, so we get the declarations later.
// It would be weird to have the functions used in the computation before the computation itself!

static void sortDriversByRouteCount(DriverAVL* drivers, DriverSortAVL** sortedDrivers);

static void printTop10Drivers(const DriverSortAVL* node, int* n);

void computationD1(RouteStream* stream)
{
    memInit(&driverStringListMem, 512 * 1024);
    memInit(&driverAVLMem, 1024 * 1024);
    memInit(&driverSortAVLMem, 1024 * 1024);

    // The AVL containing all drivers by their name, with the number of routes they have taken
    // (in extraData, with the DriverData struct), for lookup.
    //
    // This AVL contains all the strings for driver names, copied from each RouteStep.
    // The driver name strings in this AVL are entirely valid through the entire computation,
    // which avoids string duplication and memory management nightmares -- no more segfaults!
    //
    // You can think of it as a dictionary, or a function/map:
    //    f(driverName) -> routeCount
    DriverAVL* drivers = NULL;

    // The map containing all routes, for lookup.
    // Each route in this map holds a list of all drivers who have already drove this route.
    //
    // It's really just a function:
    //     f(routeId) -> [driverName1, driverName2, ...]
    RouteMap map;
    routeMapInit(&map, (1 << 16), 0.7f); // 65536 is good

    // Phase 1: Read all the route steps
    // ------------------------------------------
    // Let's read all the route steps and fill our two intermediate AVLs with data.
    // The routes AVL will be used to filter out drivers we've already seen on a particular route.
    {
        PROFILER_START("D1: Read routes and register drivers");

        RouteStep step;
        while (rsRead(stream, &step, ROUTE_ID | DRIVER_NAME))
        {
            RouteMapEntry* entry = routeMapLookup(&map, step.routeId);
            if (!entry)
            {
                entry = routeMapInsert(&map, step.routeId);
                entry->drivers = (LLStr) { .value = NULL, .next = NULL };
            }
            LLStr* driversInRoute = &entry->drivers;

            // Has the driver already been seen on this route?
            bool driverAlreadySeen = false;
            LLStr* it = driversInRoute;
            // Traverse the linked list of drivers already assigned to this route.
            while (it && it->value != NULL)
            {
                if (strcmp(it->value, step.driverName) == 0)
                {
                    driverAlreadySeen = true;
                    break;
                }
                it = it->next;
            }

            if (!driverAlreadySeen)
            {
                // If the driver has not been seen on this route, register it.
                // Insert the driver into the AVL, or use the exisiting node.
                DriverAVL* driverNode;
                driverNode = driverAVLLookup(drivers, step.driverName);
                if (!driverNode)
                {
                    drivers = driverAVLInsert(drivers, step.driverName, &driverNode, NULL);
                }

                // Increment its routeCount value.
                driverNode->routeCount++;

                // Add it to the list of drivers involved in this route.
                // Use the string located in the AVL since it is valid through the entire computation.
                char* driverName = driverNode->name;
                llStrAdd(driversInRoute, driverName);
            }
        }

        PROFILER_END();
    }

    // The AVL which will contain the drivers sorted by route count.
    // We're going to extract the 10 largest elements from it.
    DriverSortAVL* bestDrivers = NULL;

    // Phase 2: Sort the drivers by route count
    // ------------------------------------------
    // There, we're just going to insert all the drivers into a specialized AVL (DriverSortAVL).
    //
    // AVL trees have the property to be sorted at all times, so we can do a reverse in-order traversal
    // to get the 10 drivers with the most routes.
    {
        PROFILER_START("D1: Sort drivers by route count");

        sortDriversByRouteCount(drivers, &bestDrivers);

        int n = 0;
        printTop10Drivers(bestDrivers, &n);

        PROFILER_END();
    }

    // Phase 3: Free everything
    // ------------------------------------------
    // We're done, and we can just free all the AVL trees we have created.
    {
        PROFILER_START("D1: Free stuff")

        // Free the entire hash map.
        routeMapFree(&map);

        // Free all the memory arenas.
        memFree(&driverStringListMem);
        memFree(&driverAVLMem);
        memFree(&driverSortAVLMem);

        PROFILER_END();
    }
}

// Transfer all drivers from the 'drivers' AVL (which is NOT sorted according to the route count)
// to the sorting AVL, using a simple pre-order traversal.
static void sortDriversByRouteCount(DriverAVL* drivers, DriverSortAVL** sortedDrivers)
{
    if (drivers == NULL)
    {
        return;
    }

    sortDriversByRouteCount(drivers->left, sortedDrivers);

    DriverSortAVLData insertion = {
        .routesTaken = drivers->routeCount,
        .driverName = drivers->name
    };
    // We're using a double pointer so we can modify the root (in case *sortedDrivers is NULL for example).
    *sortedDrivers = driverSortAVLInsert(*sortedDrivers, &insertion, NULL, NULL);

    sortDriversByRouteCount(drivers->right, sortedDrivers);
}

// A simple in-order traversal to print the top 10 drivers and the number of routes taken.
// The n parameter is used to stop the traversal once we've printed 10 drivers, it needs to be initialized with 0.
static void printTop10Drivers(const DriverSortAVL* node, int* n)
{
    if (node == NULL || *n >= 10)
    {
        return;
    }

    printTop10Drivers(node->right, n);

    if (*n < 10)
    {
        printf("%s;%d\n", node->driverName, node->routesTaken);
        *n += 1;
    }

    printTop10Drivers(node->left, n);
}

#else
void computationD1(struct RouteStream* s)
{
    fprintf(stderr, "Cannot use computation D1 without experimental algorithms enabled!\n");
    exit(9);
}
#endif