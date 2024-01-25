#include <stdio.h>
#include <locale.h>
#include <stdlib.h>

#include "profile.h"
#include "route.h"
#include "options.h"
#include "computations/computations.h"
#ifdef WIN32
#include <windows.h>
#endif

int main(int argv, char** argc)
{
    // Avoid locale-dependant surprises when parsing floats in the CSV.
    // NOTE: We don't use atof anymore... But let's keep it *just in case* you know?
    // Actually, this is very important for strcmp to work fast and without locale-dependant
    // weirdnesses.
    setlocale(LC_ALL, "C");

    // Fix UTF-8 output for Windows.
#ifdef WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Initialise the profiler. (Does nothing if it is not enabled.)
    profilerInit();

    // Parse the options (file and computation type)
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
        fprintf(stderr, "Pas de traitement donné !\n");

        // We're exiting without closing the stream, but honestly that's not really important,
        // the process is going to exit anyway...
        return 1;
    }

    rsClose(&stream);

    return 0;
}
