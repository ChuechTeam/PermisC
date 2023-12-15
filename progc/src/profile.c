#include "profile.h"

ProfilerState profState;

void profilerInit()
{
#ifdef WIN32
    QueryPerformanceFrequency(&profState.secFreq);
#endif
}
