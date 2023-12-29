#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "computations.h"
#include "route.h"
#include "string_avl.h"
#include "profile.h"

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
void llStrAdd(LLStr* list, char* value)
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

        LLStr* newNode = malloc(sizeof(LLStr));
        assert(newNode);

        newNode->value = value;
        newNode->next = NULL;
        list->next = newNode;
    }
}

/*
 * ------------
 * ROUTE AVL
 * ------------
 */

// The AVL tree of all routes.
// Also remembers which drivers pass by each route, to avoid duplicates.
typedef struct RouteAVL
{
    AVL_HEADER(RouteAVL)

    // The route ID.
    int routeId;

    // The linked list of all drivers passing by this route.
    // This list must be freed when destroying the AVL.
    // (This is extra data, not used for comparison)
    LLStr drivers;
} RouteAVL;

RouteAVL* routeAVLCreate(const int* routeId)
{
    RouteAVL* avl = malloc(sizeof(RouteAVL));
    assert(avl); // Make sure we have enough memory.

    AVL_INIT(avl); // Initializes left, right and balance.
    avl->routeId = *routeId;
    // Initialize the linked list with zero elements (NULL value).
    avl->drivers.value = NULL;
    avl->drivers.next = NULL;

    return avl;
}

int routeAVLCompare(const RouteAVL* a, const int* routeId)
{
    return a->routeId - *routeId;
}

// Declare the routeAVLInsert function.
AVL_DECLARE_INSERT_FUNCTION(routeAVLInsert, RouteAVL, int,
                            (AVLCreateFunc) &routeAVLCreate, (AVLCompareValueFunc) &routeAVLCompare)

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

DriverSortAVL* driverSortAVLCreate(const DriverSortAVLData* data)
{
    DriverSortAVL* avl = malloc(sizeof(DriverSortAVL));
    assert(avl); // Make sure we have enough memory.

    AVL_INIT(avl); // Initializes left, right and balance.
    avl->routesTaken = data->routesTaken;
    avl->driverName = data->driverName;

    return avl;
}

// First compare the routes taken, then the driver name.
int driverSortAVLCompare(const DriverSortAVL* a, const DriverSortAVLData* data)
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

AVL_DECLARE_INSERT_FUNCTION(driverSortAVLInsert, DriverSortAVL, DriverSortAVLData,
                            (AVLCreateFunc) &driverSortAVLCreate, (AVLCompareValueFunc) &driverSortAVLCompare)

/*
 * ------------
 * THE COMPUTATION (at last!)
 * ------------
 */

// The data attached to each driver in the 'drivers' StringAVL.
typedef struct
{
    int routeCount;
} DriverData;

// Define the prototype of the functions used in computationD1 first, so we get the declarations later.
// It would be weird to have the functions used in the computation before the computation itself!

void sortDriversByRouteCount(StringAVL* drivers, DriverSortAVL** sortedDrivers);

void printTop10Drivers(const DriverSortAVL* node, int* n);

void freeBestDriversAVL(DriverSortAVL* node);

void freeRoutesAVL(RouteAVL* node);

void computationD1(RouteStream* stream)
{
    // The intermediate AVL containing all routes, for lookup.
    // Each route in this AVL holds a list of all drivers who have already drove this route.
    //
    // You can think of it as a dictionary, or a function/map:
    //     f(routeId) -> [driverName1, driverName2, ...]
    RouteAVL* routes = NULL;

    // The AVL containing all drivers by their name, with the number of routes they have taken
    // (in extraData, with the DriverData struct), for lookup.
    //
    // This AVL contains all the strings for driver names, copied from each RouteStep.
    // The driver name strings in this AVL are entirely valid through the entire computation,
    // which avoids string duplication and memory management nightmares -- no more segfaults!
    //
    // You can think of it as a dictionary, or a function/map:
    //    f(driverName) -> routeCount
    StringAVL* drivers = NULL;

    // Phase 1: Read all the route steps
    // ------------------------------------------
    // Let's read all the route steps and fill our two intermediate AVLs with data.
    // The routes AVL will be used to filter out drivers we've already seen on a particular route.
    {
        PROFILER_START("D1: Read routes and register drivers");

        RouteStep step;
        while (rsRead(stream, &step, ROUTE_ID | DRIVER_NAME))
        {
            // Insert the route into the route AVL, or find the existing node.
            RouteAVL* routeInfo = NULL;
            routes = routeAVLInsert(routes, &step.routeId, &routeInfo, NULL);

            // Has the driver already been seen on this route?
            bool driverAlreadySeen = false;
            LLStr* it = &routeInfo->drivers;
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
                StringAVL* driverNode;
                drivers = stringAVLInsert(drivers, step.driverName, NULL, &driverNode, NULL);

                // Increment its routeCount value.
                DriverData* data = driverNode->extraData;
                if (data == NULL)
                {
                    data = driverNode->extraData = malloc(sizeof(DriverData));
                    assert(data);

                    data->routeCount = 0;
                }
                data->routeCount++;

                // Add it to the list of drivers involved in this route.
                // Use the string located in the AVL since it is valid through the entire computation.
                char* driverName = driverNode->value;
                llStrAdd(&routeInfo->drivers, driverName);
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
        PROFILER_START("D1: Free trees")

        // The bestDrivers AVL can be freed easily,
        // other than its child nodes it has nothing else to free.
        freeBestDriversAVL(bestDrivers);

        // The routes AVL needs to free all of its nodes,
        // and all of the linked lists in those nodes as well.
        //
        // The strings in the linked lists do not need to be freed because they
        // belong to the drivers AVL, which we're going to free just after.
        freeRoutesAVL(routes);

        // Finally, the drivers AVL can be freed using the stringAVLFree function,
        // which is going to free all the nodes and all the extraData we've allocated.
        stringAVLFree(drivers);

        PROFILER_END();
    }
}

// Transfer all drivers from the 'drivers' AVL (which is NOT sorted according to the route count)
// to the sorting AVL, using a simple pre-order traversal.
void sortDriversByRouteCount(StringAVL* drivers, DriverSortAVL** sortedDrivers)
{
    if (drivers == NULL)
    {
        return;
    }

    sortDriversByRouteCount(drivers->left, sortedDrivers);

    DriverSortAVLData insertion = {
        .routesTaken = ((DriverData*) drivers->extraData)->routeCount,
        .driverName = drivers->value
    };
    // We're using a double pointer so we can modify the root (in case *sortedDrivers is NULL for example).
    *sortedDrivers = driverSortAVLInsert(*sortedDrivers, &insertion, NULL, NULL);

    sortDriversByRouteCount(drivers->right, sortedDrivers);
}

// A simple in-order traversal to print the top 10 drivers and the number of routes taken.
// The n parameter is used to stop the traversal once we've printed 10 drivers, it needs to be initialized with 0.
void printTop10Drivers(const DriverSortAVL* node, int* n)
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

// Free every node of the DriverSortAVL.
void freeBestDriversAVL(DriverSortAVL* node)
{
    if (node == NULL)
    {
        return;
    }

    freeBestDriversAVL(node->left);
    freeBestDriversAVL(node->right);

    free(node);
}

// Free every node of the RouteAVL, and also free its linked list.
void freeRoutesAVL(RouteAVL* node)
{
    if (node == NULL)
    {
        return;
    }

    freeRoutesAVL(node->left);
    freeRoutesAVL(node->right);

    // Free the allocated linked list nodes (second and later).
    // We do not need to free the strings as they are in the drivers AVL, which will be freed later on.
    LLStr* it = node->drivers.next;
    while (it != NULL)
    {
        LLStr* next = it->next;
        free(it);
        it = next;
    }

    free(node);
}
