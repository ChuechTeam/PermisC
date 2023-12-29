#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdbool.h>

typedef enum
{
    COMPUTATION_NONE,
    COMPUTATION_S,
    COMPUTATION_T,
    COMPUTATION_D1,
} ComptuationOption;

typedef struct {
    char* file; // Just a reference to the argv string.
    ComptuationOption computation;
} Options;

bool parseOptions(int argc, char** argv, Options* outOptions, char errMsg[256]);

#endif //OPTIONS_H
