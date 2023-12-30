#ifndef ROUTE_H
#define ROUTE_H

/*
 * route.h
 * ---------------
 * Everything needed to read the CSV file of all route steps.
 */

#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"

#define ERR_MAX 256

typedef struct RouteStep
{
    int routeId;
    int stepId;
    char* townA; // Invalidated on the next call to rsRead
    char* townB; // Invalidated on the next call to rsRead
    float distance;
    char* driverName; // Invalidated on the next call to rsRead
} RouteStep;

typedef struct RouteStream
{
    FILE* file;

    // The buffer contains multiple lines of the CSV file.
    // Each line is guaranteed to end with a '\n' character.
    char* readBuf;
    // The current read position in the buffer.
    // Set to readBufEnd when the buffer is empty OR when reaching the end of the buffer.
    char* readBufCursor;
    // The exclusive end of the buffer.
    // Points to the character just after the last character of the buffer.
    char* readBufEnd;
    uint32_t readBufChars; // The total number of characters in the buffer.

    // True when the stream has a file open, and a buffer ready.
    bool valid;
    // True when the stream has been closed using rsClose.
    bool closed;
} RouteStream;

typedef enum
{
    ROUTE_ID = 1 << 0,
    STEP_ID = 1 << 1,
    TOWN_A = 1 << 2,
    TOWN_B = 1 << 3,
    DISTANCE = 1 << 4,
    DRIVER_NAME = 1 << 5,
    ALL_FIELDS = ROUTE_ID | STEP_ID | TOWN_A | TOWN_B | DISTANCE | DRIVER_NAME
} RouteFields;

// Opens a CSV file of all routes using the given path.
// Use rsCheck to check if the stream has been created successfully,
// and get an error message if it didn't.
RouteStream rsOpen(const char* path);

// Checks the validity of a stream, and outputs an error message if it is not valid.
bool rsCheck(const RouteStream* stream, char errMsg[ERR_MAX]);

// Reads the next route step from the stream. When there are no more lines, returns false.
// Use the fieldsToRead parameter to control which fields should be read and ignored.
//
// IMPORTANT: All strings (townA, townB, driverName) are only valid during the call to rsRead.
// The next call to rsRead make them invalid.
//
// Example:
// void func(RouteStream* stream) {
//     RouteStep step;
//     while (rsRead(&stream, &step, DRIVER_NAME | DISTANCE)) {
//         printf("Looks like %s just traveled %f meters!\n", step.driverName, step.distance);
//    }
// }
bool rsRead(RouteStream* stream, RouteStep* outRouteStep, RouteFields fieldsToRead);

// Closes the file and frees any resources allocated by the stream. Marks the stream as invalid.
void rsClose(RouteStream* stream);

#endif
