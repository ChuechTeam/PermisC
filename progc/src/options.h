#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * options.h
 * ----------------
 * Simply parses arguments: the computation and the CSV file path.
 */

#include <stdbool.h>

typedef enum
{
    COMPUTATION_NONE,
    COMPUTATION_D1,
    COMPUTATION_D2,
    COMPUTATION_L,
    COMPUTATION_S,
    COMPUTATION_T,
} ComptuationOption;

typedef struct {
    char* file; // Just a reference to the argv string.
    ComptuationOption computation;
} Options;

bool parseOptions(int argc, char** argv, Options* outOptions, char errMsg[256]);

#endif //OPTIONS_H
