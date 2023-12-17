#ifndef MAGIC_STRCMP_H
#define MAGIC_STRCMP_H

// Behold... Wild experiments!


#include <immintrin.h>
#include <stdalign.h>

// Useful stuff: _mm256_testz_si256, _mm256_movemask_epi8
// Assuming a and b are of same length <= 31
// And there are lots of zeros at the end
// It had the potential to be good... But it wasn't. :(
static int myVeryCoolStrcmp(char* a, char* b)
{
    // Make two vectors containing the characters of both strings.
    __m256i aVec = _mm256_loadu_si256((__m256i*)a);
    __m256i bVec = _mm256_loadu_si256((__m256i*)b);

    // Compare each difference to 0 to know which characters are equal
    // zeros[n] = a[n] == b[n] ? 0xFF : 0x00
    __m256i zeros = _mm256_cmpeq_epi8(aVec, bVec);

    // Convert the 'zeros' vector to a mask where the n-th bit is 1 if strings are equal at the n-th character.
    // bit(n, zeroMask) = a[n] == b[n]
    uint32_t zeroMask = _mm256_movemask_epi8(zeros);

    // Just apply the NOT operator so we have 1 if there's a difference:
    // bit(n, notZeroMask) = a[n] != b[n]
    uint32_t notZeroMask = ~zeroMask;

    // CTZ = Count Trailing Zeros
    // Start from the first (least significant) bit, and find the index of the bit with a value of 1.
    // If there are no 1s, then the value is 32.
    // Example: 0b00000100 -> 2
    int differIndex = __builtin_ctz(notZeroMask);

    // Return the difference between the two differing characters.
    // If we're at the end of the string, then differIndex will be 32, and we'll return 0.
    int c1 = (unsigned char) a[differIndex], c2 = (unsigned char) b[differIndex];
    int diff = c1 - c2;
    int res = (c1|c2) == '\0' ? 0 : diff;
    //assert(res == strcmp(a,b));
    return res;
}

typedef union
{
    __m256i vec;
    int64_t int64[4];
} Vec256;

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
