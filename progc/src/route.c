#include "route.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include "delimiter_search.h"

// 128 KB
// After some profiling, it empirically works fast on my computer...
#define READ_BUFFER_SIZE 128*1024

// The slack needed for the delimiter search to work properly.
#define READ_BUFFER_SLACK 64

RouteStream rsOpen(const char* path)
{
    RouteStream s;
    s.file = NULL;
    s.readBuf = NULL;
    s.readBufChars = 0;
    s.closed = false;

    FILE* file = fopen(path, "rb");
    s.file = file;

    // Leave 64 bytes of slack for delimiter searching to work properly.
    // Make sure the buffer is zeroed out, for extra safety
    // especially for the slack which NEEDS to be zeroed.
    s.readBuf = calloc(1, READ_BUFFER_SIZE + READ_BUFFER_SLACK);
    s.readBufEnd = s.readBuf + READ_BUFFER_SIZE - 1;
    s.readBufCursor = s.readBufEnd;
    // Checked later by rsCheck

    if (file)
    {
        // Disable buffering, it's useless since we use fread with our own buffer.
        setvbuf(file, NULL, _IONBF, 0);
        // Skip the first line (header with column names)
        while (true)
        {
            // continue until we get to a new line :D
            int ch = fgetc(file);
            if (ch == EOF || ch == '\n')
            {
                break;
            }
        }
    }

    s.valid = file != NULL && s.readBuf != NULL;

    return s;
}

bool rsCheck(const RouteStream* stream, char errMsg[ERR_MAX])
{
    assert(stream && !stream->closed);

    if (!stream->file || ferror(stream->file))
    {
        char* fileError = strerror(errno);
        snprintf(errMsg,ERR_MAX, "%s", fileError);
        errMsg[ERR_MAX-1] = '\0';
        return false;
    }
    else if (!stream->readBuf)
    {
        snprintf(errMsg, ERR_MAX, "Pas assez de mémoire pour allouer le buffer de lecture");
        return false;
    }
    else {
        return true;
    }
}

// Fill the buffer with the next chunk of data.
//
// If the buffer doesn't have enough room to fit the last line entirely,
// the buffer will be truncated to the line preceding it,
// and the file cursor will be rolled back to the beginning of the line.
bool continueBufferRead(RouteStream* stream)
{
    assert(stream);

    // Reset the position cursor and end.
    stream->readBufCursor = stream->readBuf;

    uint32_t bytesRead = (uint32_t) fread(stream->readBuf, 1, READ_BUFFER_SIZE, stream->file);
    if (bytesRead == READ_BUFFER_SIZE)
    {
        // Check if the last line has been truncated, unless we're at the very end.
        // If it is the case, roll back the file cursor until the last newline character.
        //
        // NOTE: There's a very special case where we've reached the end of the file, and the last character
        // is not a newline character (because we're at the end). It shouldn't be much of a problem though,
        // we're just going to rollback the cursor and the next call will have only one line.
        if (stream->readBuf[READ_BUFFER_SIZE - 1] != '\n')
        {
            int offset = 0;
            while (stream->readBuf[READ_BUFFER_SIZE - 1 - offset] != '\n')
            {
                offset++;
            }

            fseek(stream->file, -offset, SEEK_CUR);
            stream->readBufChars = READ_BUFFER_SIZE - offset;
        }
        else
        {
            stream->readBufChars = READ_BUFFER_SIZE;
        }
        stream->readBufEnd = stream->readBuf + stream->readBufChars;

        return true;
    }
    else if (bytesRead == 0)
    {
        // No more characters, EOF! (Or error)
        stream->readBufChars = 0;
        stream->readBufEnd = stream->readBuf;

        return false;
    }
    else // if (bytesRead < READ_BUFFER_SIZE)
    {
        // The buffer is not full, so we reached the end of the file.

        // Zero out the rest of the buffer to avoid overflow.
        memset(stream->readBuf + bytesRead, 0, READ_BUFFER_SIZE - bytesRead);

        if (stream->readBuf[bytesRead - 1] == '\n')
        {
            // There's already a newline character at the end, so we're good.
            stream->readBufChars = bytesRead;
            stream->readBufEnd = stream->readBuf + stream->readBufChars;
        }
        else
        {
            // There's no newline character before EOF, so we need to add one.
            // We have the room to add a newline character at the end, so let's do it.
            stream->readBuf[bytesRead] = '\n';

            stream->readBufChars = bytesRead + 1;
            stream->readBufEnd = stream->readBuf + stream->readBufChars;
        }

        return true;
    }
}

/*
 * READ FUNCTIONS: readUInt, readStr, readFloat
 * --------------------------------------------
 * These functions exist to parse the three different types of data in the file.
 * Unsigned integers, strings and unsigned floats.
 *
 * The reasoning behind making those functions by hand instead of using stuff
 * like atoi, atof, etc. is simple: they are slow and annoying to use in sequential read.
 * atoi and atof require a null-terminated string, and that would induce really wack stuff involving
 * either copying strings or temporary null terminations...
 *
 * Since we know the exact format of integers and floats, writing them is really easy.
 */

float readUnsignedFloat(char* const start, char* const end)
{
    uint32_t intPart = 0, decPart = 0;
    uint32_t decSize = 1;

    bool dec = false;

    char* cursor = start;
    while (cursor != end)
    {
        if (*cursor != '.')
        {
            // Make sure it is a digit
            assert(*cursor >= '0' && *cursor <= '9');

            uint32_t digit = *cursor - '0';
            if (!dec)
            {
                intPart *= 10;
                intPart += digit;
            }
            else
            {
                decPart *= 10;
                decPart += digit;
                decSize *= 10;
            }
        }
        else
        {
            assert(!dec);
            dec = true;
        }

        cursor++;
    }

    return (float)intPart + (float)decPart / decSize;
}

char* readStr(char* start, char* end)
{
    *end = '\0';

    return start;
}

uint32_t readUnsignedInt(char* const start, char* const end)
{
    uint32_t number = 0;

    char* cursor = start;

    while (cursor != end)
    {
        // Make sure it is a digit
        assert(*cursor >= '0' && *cursor <= '9');

        uint32_t digit = *cursor - '0';
        number *= 10;
        number += digit;
        cursor++;
    }

    return number;
}

bool rsRead(RouteStream* stream, RouteStep* outRouteStep, RouteFields fieldsToRead)
{
    assert(outRouteStep);
    assert(stream && stream->valid);

    if (stream->readBufCursor >= stream->readBufEnd)
    {
        bool bufferSuccess = continueBufferRead(stream);
        if (!bufferSuccess)
        {
            return false;
        }
    }

    char* lineBegin = stream->readBufCursor;

    // Find all the delimiters (the 5 semicolons and the new line character)
    // searchDelimiters makes sure that the delimiters are in the right place
    // (e.g. no newline as second delimiter), and that we aren't overflowing the buffer (zero/newline checks).
    char* delimiters[6];
    searchDelimiters(lineBegin, delimiters);

    if (fieldsToRead & ROUTE_ID)
        outRouteStep->routeId = readUnsignedInt(lineBegin, delimiters[0]);

    if (fieldsToRead & STEP_ID)
        outRouteStep->stepId = readUnsignedInt(delimiters[0] + 1, delimiters[1]);

    if (fieldsToRead & TOWN_A)
        outRouteStep->townA = readStr(delimiters[1] + 1, delimiters[2]);

    if (fieldsToRead & TOWN_B)
        outRouteStep->townB = readStr(delimiters[2] + 1, delimiters[3]);

    if (fieldsToRead & DISTANCE)
        outRouteStep->distance = readUnsignedFloat(delimiters[3] + 1, delimiters[4]);

    if (fieldsToRead & DRIVER_NAME)
        outRouteStep->driverName = readStr(delimiters[4] + 1, delimiters[5]);

    stream->readBufCursor = delimiters[5] + 1;

    return true;
}

void rsClose(RouteStream* stream)
{
    assert(stream);

    if (!stream->valid)
    {
        return;
    }

    if (stream->file)
    {
        fclose(stream->file);
        stream->file = NULL;
    }
    if (stream->readBuf)
    {
        free(stream->readBuf);
        stream->readBuf = NULL;
    }

    stream->closed = true;
    stream->valid = false;
}