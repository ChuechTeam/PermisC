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

#define AVX_DELIM_SEARCH 2
#define SSE_DELIM_SEARCH 1
#define BASIC_DELIM_SEARCH 0

#ifndef DELIM_SEARCH_METHOD
#define DELIM_SEARCH_METHOD AVX_DELIM_SEARCH
#endif

#if defined(__GNUC__) || defined(__clang__)
#define d_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define d_unlikely(x) (x)
#endif

#if DELIM_SEARCH_METHOD == AVX_DELIM_SEARCH
#include <immintrin.h>

static uint32_t makeNewMask(const char* a, __m256i semiColon, __m256i newLine);
static uint64_t makeNewMask64(const char* a, __m256i semiColon, __m256i newLine);

#endif

static inline void searchDelimiters(char* a, char* delimiters[6])
{
#if DELIM_SEARCH_METHOD == AVX_DELIM_SEARCH
    __m256i semiColon = _mm256_set1_epi8(';');
    __m256i newLine = _mm256_set1_epi8('\n');

    uint64_t mask = makeNewMask64(a, semiColon, newLine);
    int deltaDiff = __builtin_ctzll(mask);
    char* line = a;
    assert(*a != '\0');

#ifdef __GNUC__
#pragma GCC unroll 6
#elif defined(__clang__)
#pragma unroll
#endif
    for (int i = 0; i < 6; ++i)
    {
        while (d_unlikely(mask == 0))
        {
            a += 64;
            line = a;
            assert(*a != '\0');

            mask = makeNewMask64(a, semiColon, newLine);
            deltaDiff = __builtin_ctzll(mask);
        }

        // For some awkward reason, putting this instruction BEFORE "line++"
        // gives a huge performance boost, and GCC generates a totally different assembly code.
        mask >>= deltaDiff + 1;
        line += deltaDiff;
        delimiters[i] = line;

        if (i == 5)
        {
            assert(*line == '\n');
        }
        else
        {
            assert(*line == ';');
        }

        line++;
        deltaDiff = __builtin_ctzll(mask);
    }
#elif DELIM_SEARCH_METHOD == BASIC_DELIM_SEARCH
    int delimNum = 0;
    while (delimNum < 6)
    {
        if (*a == ';' || *a == '\n')
        {
            delimiters[delimNum] = a;
            delimNum++;

            if (*a == ';')
            {
                assert(delimNum <= 5);
            }
            else // *a == '\n'
            {
                assert(delimNum == 6);
            }
        }
        else
        {
            assert(*a != '\0');
        }
        a++;
    }
#endif
}

#if DELIM_SEARCH_METHOD == AVX_DELIM_SEARCH
static uint32_t makeNewMask(const char* a, __m256i semiColon, __m256i newLine)
{
    // Make two vectors containing the characters of both strings.
    __m256i aVec = _mm256_loadu_si256((__m256i*)a);

    // Compare each difference to 0 to know which characters are equal
    // zeros[n] = a[n] == b[n] ? 0xFF : 0x00
    __m256i semiColonFound = _mm256_cmpeq_epi8(aVec, semiColon);
    __m256i newLineFound = _mm256_cmpeq_epi8(aVec, newLine);

    __m256i anyFound = _mm256_or_si256(semiColonFound, newLineFound);

    // Convert the 'zeros' vector to a mask where the n-th bit is 1 if strings are equal at the n-th character.
    // bit(n, foundAny) = a[n] == b[n]
    return _mm256_movemask_epi8(anyFound);
}

static uint64_t makeNewMask64(const char* a, __m256i semiColon, __m256i newLine)
{
    uint32_t lo = makeNewMask(a, semiColon, newLine);
    uint32_t hi = makeNewMask(a + 32, semiColon, newLine);

    return (uint64_t)hi << 32 | lo;
}
#endif

// Keeping the old versions of that function just for reference
// __attribute_noinline__ void myVeryCoolStrFunc(const char* a, uint16_t delimiters[6])
// {
//     uint16_t offset = 0;
//     int delimNum = 0;
//
//     __m256i semiColon = _mm256_set1_epi8(';');
//     __m256i newLine = _mm256_set1_epi8('\n');
//
//     while (true)
//     {
//         // Make two vectors containing the characters of both strings.
//         __m256i aVec = _mm256_loadu_si256((__m256i*)a);
//
//         // Compare each difference to 0 to know which characters are equal
//         // zeros[n] = a[n] == b[n] ? 0xFF : 0x00
//         __m256i semiColonFound = _mm256_cmpeq_epi8(aVec, semiColon);
//         __m256i newLineFound = _mm256_cmpeq_epi8(aVec, newLine);
//
//         __m256i anyFound = _mm256_or_si256(semiColonFound, newLineFound);
//
//         // Convert the 'zeros' vector to a mask where the n-th bit is 1 if strings are equal at the n-th character.
//         // bit(n, foundAny) = a[n] == b[n]
//         uint32_t foundAny = _mm256_movemask_epi8(anyFound);
//
//         int differIndex = __builtin_ctz(foundAny);
//         while (foundAny)
//         {
//             delimiters[delimNum] = offset + differIndex;
//
//             if (a[differIndex] == ';')
//             {
//                 assert(delimNum <= 5);
//
//                 delimNum++;
//                 foundAny &= ~(1 << differIndex);
//                 differIndex = __builtin_ctz(foundAny);
//             }
//             else
//             {
//                 return;
//             }
//         }
//         a += 32;
//         offset += 32;
//     }
// }
// __attribute_noinline__ void myVeryCoolStrFunc2(const char* a, const char* delimiters[6])
// {
//     int delimNum = 0;
//
//     __m256i semiColon = _mm256_set1_epi8(';');
//     __m256i newLine = _mm256_set1_epi8('\n');
//
//     while (true)
//     {
//         // Make two vectors containing the characters of both strings.
//         __m256i aVec = _mm256_loadu_si256((__m256i*)a);
//
//         // Compare each difference to 0 to know which characters are equal
//         // zeros[n] = a[n] == b[n] ? 0xFF : 0x00
//         __m256i semiColonFound = _mm256_cmpeq_epi8(aVec, semiColon);
//         __m256i newLineFound = _mm256_cmpeq_epi8(aVec, newLine);
//
//         __m256i anyFound = _mm256_or_si256(semiColonFound, newLineFound);
//
//         // Convert the 'zeros' vector to a mask where the n-th bit is 1 if strings are equal at the n-th character.
//         // bit(n, foundAny) = a[n] == b[n]
//         uint32_t foundAny = _mm256_movemask_epi8(anyFound);
//
//         int deltaDiff = __builtin_ctz(foundAny);
//         const char* line = a;
//         while (foundAny)
//         {
//             line += deltaDiff;
//             delimiters[delimNum] = line;
//
//             if (*line == ';')
//             {
//                 assert(delimNum <= 5);
//
//                 delimNum++;
//                 foundAny >>= deltaDiff + 1;
//                 line++;
//                 deltaDiff = __builtin_ctz(foundAny);
//             }
//             else
//             {
//                 return;
//             }
//         }
//         a += 32;
//     }
// }


#endif //DELIMITER_SEARCH_H
