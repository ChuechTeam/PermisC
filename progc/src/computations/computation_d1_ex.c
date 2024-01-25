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
#include "partition.h"

static MemArena driverStringListMem;
static MemArena driverSortAVLMem;
static MemArena driverStringsMem;

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
    bool occupied;
    uint32_t key;
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
    uint32_t a = *(uint32_t*) key;
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

static inline uint32_t* MAP_GET_KEY_PTR_FUNC(RouteMapEntry* entry, MapKeyScratch scratch)
{
    return &entry->key;
}

MAP_DECLARE_FUNCTIONS_STATIC(routeMap, RouteMapEntry, uint32_t, true)

#undef CURRENT_MAP_TYPE

/*
 * DRIVER MAP
 */

typedef struct
{
    char* str;
    uint32_t length;
} MeasuredString;

typedef struct DriverEntry
{
    char* name; // NULL if empty.
    uint32_t length;
    uint32_t routeCount;
} DriverEntry;

typedef struct DriverMap
{
    MAP_HEADER(DriverEntry)
} DriverMap;

#define CURRENT_MAP_TYPE() DriverMap

static inline uint32_t MAP_HASH_FUNC(const MeasuredString* key, uint32_t capacityExponent)
{
    uint32_t a = 0;
    for (uint32_t i = 0; i < key->length; i++)
    {
        a *= 31;
        a += key->str[i];
    }
    return a;
}

static inline bool MAP_KEY_EQUAL_FUNC(const DriverEntry* entry, const MeasuredString* key)
{
    return entry->length == key->length && memcmp(entry->name, key->str, key->length) == 0;
}

static inline bool MAP_GET_OCCUPIED_FUNC(const DriverEntry* entry)
{
    return entry->name != NULL;
}

static inline void MAP_MARK_OCCUPIED_FUNC(DriverEntry* entry, MeasuredString* key)
{
    char* copy = memAlloc(&driverStringsMem, key->length + 1);
    memcpy(copy, key->str, key->length + 1);

    entry->name = copy;
    entry->length = key->length;
}

static inline MeasuredString* MAP_GET_KEY_PTR_FUNC(DriverEntry* entry, MapKeyScratch scratch)
{
    MeasuredString* scratchStr = (MeasuredString*) scratch;

    scratchStr->str = entry->name;
    scratchStr->length = entry->length;
    return scratchStr;
}

MAP_DECLARE_FUNCTIONS_STATIC(driverMap, DriverEntry, MeasuredString, true)

#undef CURRENT_MAP_TYPE

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

static void sortDriversByRouteCount(DriverMap* drivers, DriverSortAVL** sortedDrivers);

static void printTop10Drivers(const DriverSortAVL* node, int* n);

void computationD1(RouteStream* stream)
{
    memInit(&driverStringListMem, 256 * 1024);
    memInit(&driverStringsMem, 256 * 1024);
    memInit(&driverSortAVLMem, 256 * 1024);

    // The map containing all drivers by their name, with the number of routes they have taken.
    //
    // You can think of it as a dictionary, or a function/map:
    //    f(driverName) -> routeCount
    DriverMap drivers;
    driverMapInit(&drivers, 4096, 0.75f);

    // The map containing all routes, for lookup.
    // Each route in this map holds a list of all drivers who have already drove this route.
    //
    // It's really just a function:
    //     f(routeId) -> [driverName1, driverName2, ...]
    RouteMap routes;
    routeMapInit(&routes, 8192, 0.25f); // Use 8192 as we do partitioning

    // Splits all the route steps into multiple buckets for better
    // cache locality.
    Partitioner partitioner;
    partitionerInit(&partitioner, 64, 66536);

    struct StepPart
    {
        uint32_t routeId;
        uint32_t driverLen;
        char* driverName;
    };

    // Step 1: Write all steps to partitions, and register drivers
    // ------------------------------------------
    // For better performance, we'll copy every step to partitions with similar route ids.
    // During this phase, we are also going to register all drivers into the map first,
    // so we can later compare strings very fast.
    {
        PROFILER_START("Write to partitions and register drivers");

        RouteStep step;
        while (rsRead(stream, &step, ROUTE_ID | DRIVER_NAME))
        {
            MeasuredString str = (MeasuredString){step.driverName, step.driverNameLen};
            DriverEntry* entry = driverMapLookup(&drivers, str);
            if (!entry)
            {
                entry = driverMapInsert(&drivers, str);
            }

            struct StepPart part = {step.routeId, step.driverNameLen, entry->name};
            partinitionerAddS(&partitioner, step.routeId, part);
        }

        PROFILER_END();
    }

    // Phase 2: Read all the route steps
    // ------------------------------------------
    // Let's read all the route steps and fill our two intermediate AVLs with data.
    // The routes AVL will be used to filter out drivers we've already seen on a particular route.
    {
        PROFILER_START("Read partitions and count routes per driver");

        for (Partition* partition = partitioner.partitions;
            partition != (partitioner.partitions + partitioner.numPartitions);
            ++partition)
        {
            PARTITION_ITERATE(&partitioner, partition, struct StepPart, p)
            {
                RouteMapEntry* entry = routeMapLookup(&routes, p->routeId);
                if (!entry)
                {
                    entry = routeMapInsert(&routes, p->routeId);
                    entry->drivers = (LLStr){.value = NULL, .next = NULL};
                }

                // Has the driver already been seen on this route?
                bool driverAlreadySeen = false;
                LLStr* it = &entry->drivers;
                // Traverse the linked list of drivers already assigned to this route.
                while (it && it->value != NULL)
                {
                    // Here we can use pointer comparison, as both strings refer to the ones
                    // used in the map.
                    if (it->value == p->driverName)
                    {
                        driverAlreadySeen = true;
                        break;
                    }
                    it = it->next;
                }

                if (!driverAlreadySeen)
                {
                    // Find the driver in the map, we've already added it earlier.
                    MeasuredString drivStr = {p->driverName, p->driverLen};
                    DriverEntry* driverNode = driverMapLookup(&drivers, drivStr);

                    // Increment its routeCount value.
                    driverNode->routeCount++;

                    // Add it to the list of drivers involved in this route.
                    // Use the string located in the map (precisely in the strings memory arena)
                    // since it is valid through the entire computation.
                    char* storedName = driverNode->name;
                    llStrAdd(&entry->drivers, storedName);
                }
            }

            routeMapClear(&routes, -1);
        }

        PROFILER_END();
    }

    // The AVL which will contain the drivers sorted by route count.
    // We're going to extract the 10 largest elements from it.
    DriverSortAVL* bestDrivers = NULL;

    // Phase 3: Sort the drivers by route count
    // ------------------------------------------
    // There, we're just going to insert all the drivers into a specialized AVL (DriverSortAVL).
    //
    // AVL trees have the property to be sorted at all times, so we can do a reverse in-order traversal
    // to get the 10 drivers with the most routes.
    {
        PROFILER_START("Sort drivers by route count");

        sortDriversByRouteCount(&drivers, &bestDrivers);

        int n = 0;
        printTop10Drivers(bestDrivers, &n);

        PROFILER_END();
    }

    // Phase 4: Free everything
    // ------------------------------------------
    // We're done, and we can just free all the AVL trees we have created.
    {
        PROFILER_START("Free stuff")

        // Free the entire hash map.
        routeMapFree(&routes);
        driverMapFree(&drivers);

        // Free all the memory arenas.
        memFree(&driverStringListMem);
        memFree(&driverStringsMem);
        memFree(&driverSortAVLMem);

        PROFILER_END();
    }
}

// Transfer all drivers from the drivers map
// to the sorting AVL, using a simple pre-order traversal.
static void sortDriversByRouteCount(DriverMap* drivers, DriverSortAVL** sortedDrivers)
{
    for (uint32_t i = 0; i < drivers->capacity; ++i)
    {
        DriverEntry* entry = &drivers->entries[i];

        if (entry->name != NULL)
        {
            DriverSortAVLData insertion = {
                .routesTaken = entry->routeCount,
                .driverName = entry->name
            };
            // We're using a double pointer so we can modify the root (in case *sortedDrivers is NULL for example).
            *sortedDrivers = driverSortAVLInsert(*sortedDrivers, &insertion, NULL, NULL);
        }
    }
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
