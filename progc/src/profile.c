#include "profile.h"

#if ENABLE_PROFILER

ProfilerState profState;

void profilerInit()
{
#ifdef WIN32
    QueryPerformanceFrequency(&profState.secFreq);
#endif
}

#endif