#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "route.h"
#include "string_avl.h"
#include "options.h"
#include "computations/computations.h"
#ifdef WIN32
#include <windows.h>
#endif

int avlSize(const StringAVL* avl)
{
    if (avl == NULL)
        return 0;
    return avlSize(avl->left) + avlSize(avl->right) + 1;
}

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

    // Parse the options (file, computation type, etc.)
    Options options;
    char optionsErrMsg[256];
    if (!parseOptions(argv, argc, &options, optionsErrMsg))
    {
        fprintf(stderr, "Erreur d'argument : %s\n", optionsErrMsg);
        return 2;
    }

    RouteStream stream = rsOpen(options.file);

    char streamErrMsg[ERR_MAX];
    if (!rsCheck(&stream, streamErrMsg))
    {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier : %s\n", streamErrMsg);
        return 1;
    }

    if (options.computation == COMPUTATION_S)
    {
        computationS(&stream);
    }
    else if (options.computation == COMPUTATION_T)
    {
        computationT(&stream);
    }
    else if (options.computation == COMPUTATION_D1)
    {
        computationD1(&stream);
    }
    else if (options.computation == COMPUTATION_D2)
    {
        computationD2(&stream);
    }
    else if (options.computation == COMPUTATION_L)
    {
        computationL(&stream);
    }
    else
    {
        printf("Pas de traitement, lancement du petit programme de test qui a rien à voir\n");

        StringAVL* driversAVL = NULL;
        StringAVL* townsAVL = NULL;

        RouteStep step = {0, 0, NULL, NULL, 0.0f, NULL};
        PROFILER_START("Read all steps + AVL insertion");
        while (rsRead(&stream, &step, ALL_FIELDS))
        {
            driversAVL = stringAVLInsert(driversAVL, step.driverName, NULL, NULL, NULL);
            townsAVL = stringAVLInsert(townsAVL, step.townA, NULL, NULL, NULL);
            townsAVL = stringAVLInsert(townsAVL, step.townB, NULL, NULL, NULL);
        }
        PROFILER_END();

        printf("Conducteurs : %d | Villes : %d\n", avlSize(driversAVL), avlSize(townsAVL));
    }

    rsClose(&stream);

    return 0;
}
