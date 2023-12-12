#include "route.h"
#include "assert.h"
#include "stdlib.h"

RouteStream rsOpen(const char *path)
{
    RouteStream s; 
    s.file = NULL;

    FILE *file = fopen(path, "r");
    s.file = file;

    if (file) {
        // Skip the first line (header with column names)
        while (true) {
            // continue until we get to a new line :D
            int ch = fgetc(file);
            if (ch == EOF || ch == '\n') {
                 break;
            } 
        }
    }

    return s;
}

bool rsCheck(const RouteStream *stream, char errMsg[ERR_MAX])
{
    if (stream->file)
    {
        return true;
    }
    else
    {
        // TODO: better err msg
        errMsg = "Ca s'est mal passé.";
        return false;
    }
}

bool rsRead(RouteStream *stream, RouteStep *outRouteStep)
{
    assert(stream);
    assert(outRouteStep);
    assert(stream->file);

    // End of file or error
    if (feof(stream->file)) {
        return false;
    }

    char field[512];
    int fieldCurIdx = 0;

    int ch;
    int fieldNum = 0;
    while (true) {
        ch = fgetc(stream->file);
        if (ch == EOF || ch == '\n') {
            break;
        }
        else if (ch == ';') {
            // Parse field
            field[fieldCurIdx] = '\0';

            switch (fieldNum)
            {
            case 0:
                ; // for some reason this semicolon is necessary (the C standard is wild)
                int routeId = atoi(field);
                outRouteStep->routeId = routeId;
                break;  
            
            default:
                printf("Not implemented!!!! (field=%d) fix your stuff!!\n", fieldNum);
                break;
            }

            fieldCurIdx = 0;
            fieldNum++;
        } else {
            field[fieldCurIdx] = ch;
            fieldCurIdx++;
        }
    }

    // Not enough fields!
    assert(fieldNum == 5);

    return true;
}