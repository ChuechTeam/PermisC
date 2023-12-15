#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "route.h"
#include "string_avl.h"
#ifdef WIN32
#include <windows.h>
#endif

int main(int argv, char** argc)
{
    // Avoid locale-dependant surprises when parsing floats in the CSV.
    // NOTE: We don't use atof anymore... But let's keep it *just in case* you know?
    setlocale(LC_ALL, "C");

    // Fix UTF-8 output for Windows.
#ifdef WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Initialise the profiler.
    profilerInit();

    if (argv != 2)
    {
        printf("Nombre d'arguments invalide.\n");
        return 1;
    }

    RouteStream stream = rsOpen(argc[1]);

    char errMsg[ERR_MAX];
    if (!rsCheck(&stream, errMsg))
    {
        printf("Erreur: %s\n", errMsg);
        return 1;
    }

    StringAVL* avl = NULL;

    RouteStep step = {0, 0, NULL, NULL, 0.0f, NULL};
    uint32_t acc=0;
    PROFILER_START("Read all steps + AVL");
    while (rsRead(&stream, &step))
    {
        StringAVL* alreadyExist=NULL;
        avl = stringAVLInsert(avl, step.driverName, &alreadyExist);
        if (!alreadyExist)
        {
            acc++;
        }
    }
    PROFILER_END();

    printf("nb de personnes: %d\n", acc);

    return 0;
}
