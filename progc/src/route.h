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
    uint32_t readBufChars;
    uint32_t readBufPos;
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

// TODO: Close and free functions

RouteStream rsOpen(const char* path);
bool rsCheck(const RouteStream* stream, char errMsg[ERR_MAX]);

// Reads the next route step from the stream. When there are no more lines, returns false.
//
// IMPORTANT: All strings (townA, townB, driverName) are only valid during the call to rsRead.
// The next call to rsRead make them invalid.
bool rsRead(RouteStream* stream, RouteStep* outRouteStep, RouteFields fieldsToRead);

#endif
