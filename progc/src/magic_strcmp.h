#ifndef MAGIC_STRCMP_H
#define MAGIC_STRCMP_H

// Behold... Wild experiments!


#include <immintrin.h>
#include <stdalign.h>
#include <assert.h>
#include <string.h>

static int min(int a, int b) { return a < b ? a : b; }

// Assuming a and b are of length <= 31,
// Currently, both strings need to be on 32 bytes of accessible memory (to prevent segfaults).
static int myVeryCoolStrcmp(char* a, char* b, int lenA, int lenB)
{
    // Stop at index min(lenA, lenB)+1
    uint32_t maskBoundary = (1 << min(lenA, lenB));

    // Make two vectors containing the characters of both strings.
    __m256i aVec = _mm256_loadu_si256((__m256i*)a);
    __m256i bVec = _mm256_loadu_si256((__m256i*)b);

    // Compare each difference to 0 to know which characters are equal
    // zeros[n] = a[n] == b[n] ? 0xFF : 0x00
    __m256i zeros = _mm256_cmpeq_epi8(aVec, bVec);

    // Convert the 'zeros' vector to a mask where the n-th bit is 1 if strings are equal at the n-th character.
    // bit(n, zeroMask) = a[n] == b[n]
    uint32_t zeroMask = _mm256_movemask_epi8(zeros);

    // Just apply the NOT operator so we have 1 if there's a difference.
    // The mask boundary will flip the bit containing 
    // the last character of either A or B to 1, so we don't 
    // bit(n, notZeroMask) = a[n] != b[n]
    uint32_t notZeroMask = ~zeroMask | maskBoundary;

    // CTZ = Count Trailing Zeros
    // Start from the first (least significant) bit, and find the index of the bit with a value of 1.
    // If there are no 1s, then the value is 32.
    // Example: 0b00000100 -> 2
    int differIndex = __builtin_ctz(notZeroMask);

    // Return the difference between the two differing characters.
    // If we're at the end of the string, then differIndex will be 32, and we'll return 0.
    int c1 = (unsigned char) a[differIndex], c2 = (unsigned char) b[differIndex];
    int diff = c1 - c2;
    return diff;
}

// This one's pretty busted but maybe useful one day?
// static int myVeryCoolStrcmp(char* a, char* b)
// {
//     __m256i aVec = _mm256_loadu_si256((__m256i*)a);
//     __m256i bVec = _mm256_loadu_si256((__m256i*)b);
//
//
//     __m256i shuffleMask = _mm256_set_epi8(
//         8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
//         8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7
//     );
//     aVec = _mm256_shuffle_epi8(aVec, shuffleMask);
//     bVec = _mm256_shuffle_epi8(bVec, shuffleMask);
//
//
//     return 0;
// }
#endif //MAGIC_STRCMP_H
