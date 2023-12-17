#include "route.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

// 128 KB
// After some profiling, it empirically works fast on my computer...
#define READ_BUFFER_SIZE 128*1024

RouteStream rsOpen(const char* path)
{
    RouteStream s;
    s.file = NULL;
    s.readBuf = NULL;
    s.readBufPos = 0;
    s.readBufChars = 0;

    FILE* file = fopen(path, "rb");
    s.file = file;

    s.readBuf = malloc(READ_BUFFER_SIZE);
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

    return s;
}

bool rsCheck(const RouteStream* stream, char errMsg[ERR_MAX])
{
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
    {
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

    // Reset the position cursor.
    stream->readBufPos = 0;

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

        return true;
    }
    else if (bytesRead == 0)
    {
        // No more characters, EOF! (Or error)
        stream->readBufChars = 0;
        return false;
    }
    else // if (bytesRead < READ_BUFFER_SIZE)
    {
        // The buffer is not full, so we reached the end of the file.
        // We have the room to add a newline character at the end, so let's do it.
        stream->readBuf[bytesRead] = '\n';
        stream->readBufChars = bytesRead + 1;

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
 *
 * Each read function automatically skips the next semicolon for reading the next field.
 */

uint32_t readUnsignedInt(RouteStream* stream)
{
    uint32_t number = 0;
    uint32_t mult = 1;

    char ch = stream->readBuf[stream->readBufPos];
    while (ch != ';' && ch != '\n')
    {
        // Make sure it is a digit
        assert(ch >= '0' && ch <= '9');

        uint32_t digit = ch - '0';
        number += digit * mult;
        mult *= 10;

        stream->readBufPos += 1;
        ch = stream->readBuf[stream->readBufPos];
    }

    if (ch == ';')
    {
        stream->readBufPos++;
    }

    return number;
}

// Reads the next string in the CSV file from the stream.
// The semicolon or newline character will be replaced by a null-terminator.
char* readStr(RouteStream* stream, int32_t poolIdx, int32_t* outLen)
{
    // Find the length of the string
    char ch = stream->readBuf[stream->readBufPos];
    size_t i = 0;
    while (ch != ';' && ch != '\n')
    {
        i++;
        stream->readBufPos++;
        ch = stream->readBuf[stream->readBufPos];
    }

    // Mark the end of the string here.
    char* strStart = stream->readBuf + stream->readBufPos - i;
    stream->readBuf[stream->readBufPos] = '\0';

    // Skip the semicolon.
    if (ch == ';')
    {
        stream->readBufPos++;
    }

    *outLen = i;
    if (i < 32)
    {
        strncpy(stream->tempStrings[poolIdx], strStart, 33);
        return stream->tempStrings[poolIdx];
    }
    else
    {
        return strStart;
    }
}

float readUnsignedFloat(RouteStream* stream)
{
    uint32_t intPart = 0, decPart = 0;
    uint32_t multInt = 1, multDec = 1;

    bool dec = false;

    char ch = stream->readBuf[stream->readBufPos];
    while (ch != ';' && ch != '\n')
    {
        if (ch != '.')
        {
            // Make sure it is a digit
            assert(ch >= '0' && ch <= '9');

            uint32_t digit = ch - '0';
            if (!dec)
            {
                intPart += digit * multInt;
                multInt *= 10;
            }
            else
            {
                decPart += digit * multDec;
                multDec *= 10;
            }
        }
        else
        {
            assert(!dec);
            dec = true;
        }

        stream->readBufPos += 1;
        ch = stream->readBuf[stream->readBufPos];
    }

    float number = (float)intPart + (float)decPart / multDec;

    if (ch == ';')
    {
        stream->readBufPos++;
    }

    return number;
}

void skipField(RouteStream* stream)
{
    while (stream->readBuf[stream->readBufPos] != ';' && stream->readBuf[stream->readBufPos] != '\n')
    {
        stream->readBufPos++;
    }
    if (stream->readBuf[stream->readBufPos] == ';')
    {
        stream->readBufPos++;
    }
}

bool rsRead(RouteStream* stream, RouteStep* outRouteStep, RouteFields fieldsToRead)
{
    assert(outRouteStep);
    assert(stream);
    assert(stream->file);

    if (stream->readBufPos == stream->readBufChars - 1 || stream->readBufChars == 0)
    {
        bool bufferSuccess = continueBufferRead(stream);
        if (!bufferSuccess)
        {
            return false;
        }
    }

    memset(stream->tempStrings, 0, sizeof(stream->tempStrings));

    if (fieldsToRead & ROUTE_ID)
        outRouteStep->routeId = readUnsignedInt(stream);
    else
        skipField(stream);

    if (fieldsToRead & STEP_ID)
        outRouteStep->stepId = readUnsignedInt(stream);
    else
        skipField(stream);

    if (fieldsToRead & TOWN_A)
        outRouteStep->townA = readStr(stream, 0, &outRouteStep->townALen);
    else
        skipField(stream);

    if (fieldsToRead & TOWN_B)
        outRouteStep->townB = readStr(stream, 1, &outRouteStep->townBLen);
    else
        skipField(stream);

    if (fieldsToRead & DISTANCE)
        outRouteStep->distance = readUnsignedFloat(stream);
    else
        skipField(stream);

    if (fieldsToRead & DRIVER_NAME)
        outRouteStep->driverName = readStr(stream, 2, &outRouteStep->driverLen);
    else
        skipField(stream);

    // Can be replaced by a null character due to readStr
    assert(stream->readBuf[stream->readBufPos] == '\0' || stream->readBuf[stream->readBufPos] == '\n');

    // Advance to the next line
    if (stream->readBufPos != stream->readBufChars - 1)
    {
        stream->readBufPos++;
    }

    return true;
}

bool rsReadAllButDistance(RouteStream* stream, RouteStep* outRouteStep)
{
    return rsRead(stream, outRouteStep, ALL_FIELDS & ~DISTANCE);
}
