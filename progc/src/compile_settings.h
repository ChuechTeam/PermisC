#ifndef COMPILE_SETTINGS_H
#define COMPILE_SETTINGS_H

/*
 * compile_settings.h
 * -----------------------
 * Contains all the compile-time settings configured by the makefile.
 * Puts safe defaults if there's no definitions.
 */

#ifndef EXPERIMENTAL_ALGO
#define EXPERIMENTAL_ALGO 0
#endif

#ifndef EXPERIMENTAL_ALGO_AVX
#define EXPERIMENTAL_ALGO_AVX 0
#endif

#endif //COMPILE_SETTINGS_H
