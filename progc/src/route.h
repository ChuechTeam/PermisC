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

typedef struct RouteStep {
    int routeId;
    int stepId;
    char* townA; // malloc'd!
    char* townB; // malloc'd!
    float distance;
    char* driverName; // malloc'd!
} RouteStep;

typedef struct RouteStream {
    FILE* file;

    // The buffer contains multiple lines of the CSV file.
    // Each line is guaranteed to end with a '\n' character.
    char* readBuf;
    uint32_t readBufChars;
    uint32_t readBufPos;
} RouteStream;

RouteStream rsOpen(const char* path);
bool rsCheck(const RouteStream* stream, char errMsg[ERR_MAX]);
bool rsRead(RouteStream* stream, RouteStep* outRouteStep);

void stepFree(RouteStep* step);

#endif