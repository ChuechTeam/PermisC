#include "options.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

bool comptuationAlreadySet(char* arg, const Options* options, char errMsg[256])
{
    if (options->computation != COMPUTATION_NONE)
    {
        snprintf(errMsg, 256, "Argument « %s » invalide : traitement déjà spécifié", arg);
        return true;
    }
    else
    {
        return false;
    }
}

bool parseOptions(int argc, char** argv, Options* outOptions, char errMsg[256])
{
    assert(outOptions);
    assert(argv);

    outOptions->file = NULL;
    outOptions->computation = COMPUTATION_NONE;

    for (int i = 1; i < argc; ++i)
    {
        char* arg = argv[i];

        if (arg[0] == '-')
        {
            if (strcmp(arg, "-t") == 0)
            {
                if (comptuationAlreadySet(arg, outOptions, errMsg)) { return false; }
                outOptions->computation = COMPUTATION_T;
            }
            else if (strcmp(arg, "-s") == 0)
            {
                if (comptuationAlreadySet(arg, outOptions, errMsg)) { return false; }
                outOptions->computation = COMPUTATION_S;
            }
            else if (strcmp(arg, "-d1") == 0)
            {
                if (comptuationAlreadySet(arg, outOptions, errMsg)) { return false; }
                outOptions->computation = COMPUTATION_D1;
            }
            else if (strcmp(arg, "-d2") == 0)
            {
                if (comptuationAlreadySet(arg, outOptions, errMsg)) { return false; }
                outOptions->computation = COMPUTATION_D2;
            }
            else if (strcmp(arg, "-l") == 0)
            {
                if (comptuationAlreadySet(arg, outOptions, errMsg)) { return false; }
                outOptions->computation = COMPUTATION_L;
            }
            else
            {
                snprintf(errMsg, 256, "Option inconnue : « %s »", arg);
                return false;
            }
        }
        else
        {
            if (outOptions->file == NULL)
            {
                outOptions->file = arg;
            }
            else
            {
                snprintf(errMsg, 256, "Argument inattendu : « %s »", arg);
            }
        }
    }

    if (outOptions->file == NULL)
    {
        snprintf(errMsg, 256, "Aucun fichier spécifié");
        return false;
    }
    // No computation can be useful for debugging, let's leave it allowed for now.

    return true;
}
