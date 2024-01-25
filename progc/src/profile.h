#ifndef PROFILE_H
#define PROFILE_H

/*
 * profile.h
 * ---------------
 * Some functions to measure nanosecond time to do really basic profiling.
 */

#include "compile_settings.h"

#if ENABLE_PROFILER

#include <stdint.h>
#ifdef WIN32
#include <windows.h>
#elif defined(__unix__)
#include <unistd.h>
#include <time.h>
#endif

typedef struct
{
#ifdef WIN32
    LARGE_INTEGER secFreq;
#else
    int dummy;
#endif
} ProfilerState;

extern ProfilerState profState;

void profilerInit();

static int64_t nanos()
{
#ifdef WIN32
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);

    return count.QuadPart * 1000000000 / profState.secFreq.QuadPart;
#elif defined(__unix__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000000000 + ts.tv_nsec;
#else
#warning Profiling not supported!
    return 1;
#endif
}

#define PROFILER_START(name) int64_t pstart_ = nanos(); const char* pname_ = name;
#define PROFILER_END() int64_t pend_ = nanos(); fprintf(stderr, "[PROFILER] %s: %lld µs\n", pname_, (long long)(pend_ - pstart_)/1000);

#else

#define PROFILER_START(name)
#define PROFILER_END()
static void profilerInit() {}

#endif

#endif
