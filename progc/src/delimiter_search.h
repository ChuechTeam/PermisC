#ifndef DELIMITER_SEARCH_H
#define DELIMITER_SEARCH_H

/*
 * delimiter_search.h
 * -----------------------
 * Literally just searches the '\n' and ';' delimiters.
 * Sounds boring, but it's actually the major part of the parsing operation, which can take a lot of time!
 * Hence why this implementation is optimized using AVX or SSE instructions,
 * which can be chosen by configuring the DELIM_SEARCH_METHOD macro.
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "compile_settings.h"

// Use the AVX-optimized delimiter search only when compiling with AVX2 support. (-mavx2 argument),
// and when the EXPERIMENTAL_ALGO_AVX macro is defined.
#ifndef USE_AVX_DELIM_SEARCH
    #if EXPERIMENTAL_ALGO_AVX
        #ifdef __AVX2__
            #define USE_AVX_DELIM_SEARCH 1
        #else
            #define USE_AVX_DELIM_SEARCH 0
        #endif
    #else
        #define USE_AVX_DELIM_SEARCH 0
    #endif
#endif

#if USE_AVX_DELIM_SEARCH
#include <immintrin.h>
static uint64_t makeNewMask64(const char* a, __m256i semiColon, __m256i newLine);

// Define the "d_unlikely" macro to hint the compiler that the condition is unlikely to be true.
#if defined(__GNUC__) || defined(__clang__)
    #define d_unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define d_unlikely(x) (x)
#endif // defined(__GNUC__) || defined(__clang__)
#endif // USE_AVX_DELIM_SEARCH

// Finds the location of all the delimiters in a CSV line for route steps.
// Exits the program if the line is invalid.
//
// Assumes that the string is necessarily terminated by a newline or a null character,
// even in case of an invalid line.
//
// IMPORTANT: The 64 bytes of memory after the string ends must be allocated and zeroed out.
static inline void searchDelimiters(char* a, char* delimiters[6])
{
    // Non-AVX version. Uses strchr which *is* fairly quick (because it uses AVX).
    // We could've used strtok for this... But the way it works is fairly weird and having
    // more control is a nice plus.
#if !USE_AVX_DELIM_SEARCH
    // Find the very end of the line. Also make sure the line always ends with a newline character,
    // as this is enforced while reading the file (see route.c).
    // Worst case scenario: the file is a mess and there's a newline at the very end, but that's very rare.
    char* nextNewLine = strchr(a, '\n');
    delimiters[5] = nextNewLine;
    assert(nextNewLine);

    // Instruct the compiler to unroll the loop here, it helps quicken things a little bit.
#ifdef __GNUC__
#pragma GCC unroll 5
#endif
    for (int i = 0; i < 5; ++i)
    {
        char* nextSemi = strchr(a, ';');
        delimiters[i] = nextSemi;
        // Make sure that either:
        // - The line isn't incomplete: we can indeed find the ';' delimiter.
        // - The line isn't split up or incomplete: the newline character is after the ';' char.
        assert(nextSemi != NULL && nextNewLine > nextSemi);

        a = delimiters[i] + 1;
    }
#else
    // AVX version. 256-bit vectors are used to locate where the delimiters (; and \n) are.
    // We use __builtin_ctzll to find the index of the first delimiter.
    // The mask is then shifted to the right to find the next delimiter, and so on.
    __m256i semiColon = _mm256_set1_epi8(';');
    __m256i newLine = _mm256_set1_epi8('\n');

    uint64_t mask = makeNewMask64(a, semiColon, newLine);
    int skipNextDelimOffset = __builtin_ctzll(mask)+1;
    char* line = a-1;
    assert(*a != '\0');

    // Instruct the compiler to unroll this loop, it seems to be a bit faster.
#ifdef __GNUC__
#pragma GCC unroll 6
#endif
    for (int i = 0; i < 6; ++i)
    {
        while (d_unlikely(mask == 0))
        {
            a += 64;
            line = a-1;
            assert(*a != '\0');

            mask = makeNewMask64(a, semiColon, newLine);
            skipNextDelimOffset = __builtin_ctzll(mask)+1;
        }

        // For some awkward reason, putting this instruction at the start
        // gives a huge performance boost, and GCC generates a totally different assembly code.
        mask >>= skipNextDelimOffset;
        line += skipNextDelimOffset;
        delimiters[i] = line;

        if (i == 5)
        {
            assert(*line == '\n');
        }
        else
        {
            assert(*line == ';');
        }

        skipNextDelimOffset = __builtin_ctzll(mask)+1;
    }
#endif
}

#if USE_AVX_DELIM_SEARCH
static uint32_t makeNewMask(const char* a, __m256i semiColon, __m256i newLine)
{
    // Load the 32 characters of the string a into a vector.
    __m256i aVec = _mm256_loadu_si256((__m256i*)a);

    // Compare the string vector to one filled with ';' or '\n' characters to locate delimiters
    // semiColonFound[n] = a[n] == ';' ? 0xFF : 0x00
    __m256i semiColonFound = _mm256_cmpeq_epi8(aVec, semiColon);
    // newLineFound[n] = a[n] == '\n' ? 0xFF : 0x00
    __m256i newLineFound = _mm256_cmpeq_epi8(aVec, newLine);

    // Combine both vectors using the bitwise OR.
    // anyFound[n] = semiColonFound[n] | newLineFound[n]
    __m256i anyFound = _mm256_or_si256(semiColonFound, newLineFound);

    // Convert the 'anyFound' vector to a mask where the n-th bit is 1 if the n-th char is equal to ';' or '\n'
    // bit(n) = a[n] == ';' || a[n] == '\n'
    return _mm256_movemask_epi8(anyFound);
}

static uint64_t makeNewMask64(const char* a, __m256i semiColon, __m256i newLine)
{
    // Do the same thing as makeNewMask, but with 64 characters now!

    uint32_t lo = makeNewMask(a, semiColon, newLine);
    uint32_t hi = makeNewMask(a + 32, semiColon, newLine);

    return (uint64_t)hi << 32 | lo;
}
#endif

#endif //DELIMITER_SEARCH_H
