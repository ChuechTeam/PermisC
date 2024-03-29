#ifndef MAP_H
#define MAP_H

/*
 * map.h
 * ---------------
 * A generic and CHAOTIC map implementation, using open addressing.
 * It uses power-of-two array sizes, for faster modulo operations.
 *
 * Uses a bunch of very CURSED macros to get the generic code generated.
 * This is obviously EXPERIMENTAL and shouldn't be used in standard implementations.
 */

#include "compile_settings.h"

#if EXPERIMENTAL_ALGO

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

#define MAP_HEADER(type) type* entries; \
    uint32_t capacity; /* Must be a power of two */ \
    uint32_t capacityExponent; \
    uint32_t size; \
    float loadFactor; \
    uint32_t sizeThreshold; \

// This map works using open addressing.
typedef struct
{
    MAP_HEADER(void)
} Map;

typedef uint8_t MapKeyScratch[32];

struct AlignedScratch
{
    alignas(8) MapKeyScratch data;
};

typedef uint32_t (*HashFunc)(const void* key, uint32_t capacityExponent);
typedef bool (*KeyEqualFunc)(const void* entryA, void* key);
typedef bool (*GetOccupiedFunc)(const void* entry);
typedef void (*MarkOccupiedFunc)(void* entry, void* key);
typedef void* (*GetKeyPtrFunc)(const void* entry, MapKeyScratch scratch);

typedef struct
{
    size_t entrySize;
    HashFunc hashFunc;
    KeyEqualFunc keyEqualFunc;
    GetOccupiedFunc getOccupiedFunc;
    MarkOccupiedFunc markOccupiedFunc;
    GetKeyPtrFunc getKeyPtrFunc;
} MapMeta;

static inline void mapInit(Map* map, uint32_t initialCapacity, float loadFactor, MapMeta meta)
{
    assert((initialCapacity & (initialCapacity-1)) == 0 && "Must be a power of 2!");
    assert(map);
    assert(loadFactor > 0.0f && loadFactor < 1.0f);

    map->capacity = initialCapacity;
    map->capacityExponent = 0;
    map->size = 0;
    map->loadFactor = loadFactor;
    map->sizeThreshold = (int) (initialCapacity * loadFactor);

    // Find the n that equals initialCapacity = 2^n
    // I could use bsf, but i don't want to add more builtins...
    uint32_t expCalc = initialCapacity >> 1;
    while (expCalc != 0)
    {
        map->capacityExponent++;
        expCalc >>= 1;
    }

    map->entries = calloc(initialCapacity, meta.entrySize);
    assert(map->entries);
}

#ifndef MAP_DIAG
#define MAP_DIAG 0
#endif

#if MAP_DIAG
static uint64_t findCalls = 0;
static uint64_t findIter = 0;
#endif

static inline void* mapFindEntry(Map* map, void* key, const MapMeta meta)
{
#if MAP_DIAG
    findCalls++;
    findIter++;
#endif

    uint8_t* entries = map->entries;

    uint32_t i = meta.hashFunc(key, map->capacityExponent) & (map->capacity-1);
    // Continue searching if we come across an occupied slot by some other key
    while (meta.getOccupiedFunc(entries + meta.entrySize * i) &&
        !meta.keyEqualFunc(entries + meta.entrySize * i, key))
    {
        i = (i + 1) & (map->capacity - 1);
#if MAP_DIAG
        findIter++;
#endif
    }

#if MAP_DIAG
    if (findCalls % 100000 == 0)
    {
        fprintf(stderr, " [MAP STATS] avg=%f\n", (float)findIter / (float)findCalls);
    }
#endif

    return entries + meta.entrySize * i;
}

static inline void* mapLookup(Map* map, void* key, MapMeta meta)
{
    void* entry = mapFindEntry(map, key, meta);
    if (meta.getOccupiedFunc(entry))
    {
        return entry;
    }
    else
    {
        return NULL;
    }
}

static void mapGrow(Map* map, MapMeta meta);

static inline void* mapInsert(Map* map, void* key, const MapMeta meta)
{
    // If inserting this element would exceed the load factor threshold, grow!
    if (map->size + 1 >= map->sizeThreshold)
    {
        mapGrow(map, meta);
    }

    void* entry = mapFindEntry(map, key, meta);
    assert(!meta.getOccupiedFunc(entry));

    meta.markOccupiedFunc(entry, key);
    map->size++;

    return entry;
}

static void mapGrow(Map* map, const MapMeta meta)
{
    uint32_t prevCapacity = map->capacity;
    uint32_t nextCapacity = map->capacity * 2;
    uint32_t nextExponent = map->capacityExponent + 1;
    while ((uint32_t) (nextCapacity * map->loadFactor) <= map->size + 1)
    {
        nextCapacity *= 2;
        nextExponent++;
    }
    void* prevSlots = map->entries;
    void* nextSlots = calloc(nextCapacity, meta.entrySize);

    map->capacity = nextCapacity;
    map->capacityExponent = nextExponent;
    map->sizeThreshold = (int) (nextCapacity * map->loadFactor);
    map->entries = nextSlots;

    // Put all elements back in the new slots.
    for (uint32_t i = 0; i < prevCapacity; ++i)
    {
        void* entry = ((uint8_t*) prevSlots + meta.entrySize * i);
        if (meta.getOccupiedFunc(entry))
        {
            struct AlignedScratch scratch;
            void* k = meta.getKeyPtrFunc(entry, scratch.data);
            void* newEntry = mapFindEntry(map, k, meta);

            // Just copy the entry to its new slot.
            memcpy(newEntry, entry, meta.entrySize);
        }
    }

    free(prevSlots);
}

static void mapClear(Map* map, const int32_t newCapacity, const MapMeta meta)
{
    assert(map);

    map->size = 0;

    if (newCapacity == -1)
    {
        // Don't change the capacity
        memset(map->entries, 0, map->capacity * meta.entrySize);
    }
    else
    {
        assert((newCapacity & (newCapacity - 1)) == 0 && "Must be a power of 2!");

        if (newCapacity > map->capacity)
        {
            free(map->entries);
            map->entries = calloc(newCapacity, meta.entrySize);
        }
        else
        {
            memset(map->entries, 0, map->capacity * meta.entrySize);
        }
        // Else, don't try to free to allocate a smaller buffer.
        // Now that's maybe confusing because it won't free memory... But useless mallocs are best avoided.

        map->capacity = newCapacity;

        map->capacityExponent = 0;
        uint32_t expCalc = newCapacity >> 1;
        while (expCalc != 0)
        {
            map->capacityExponent++;
            expCalc >>= 1;
        }
    }
}

/*
 * Macro chaos!
 */

// #define CURRENT_MAP_TYPE <Put a map struct name!>

#define M_CONCAT2(A,B) A##B
#define M_CONCAT1(A,B) M_CONCAT2(A,B)
#define M_CONCAT(A,B) M_CONCAT1(A,B)

#define MAP_HASH_FUNC_NAME() M_CONCAT(CURRENT_MAP_TYPE(), _hash)
#define MAP_KEY_EQUAL_FUNC_NAME() M_CONCAT(CURRENT_MAP_TYPE(), _keyEqual)
#define MAP_GET_OCCUPIED_FUNC_NAME() M_CONCAT(CURRENT_MAP_TYPE(), _getOccupied)
#define MAP_MARK_OCCUPIED_FUNC_NAME() M_CONCAT(CURRENT_MAP_TYPE(), _markOccupied)
#define MAP_GET_KEY_PTR_FUNC_NAME() M_CONCAT(CURRENT_MAP_TYPE(), _getKeyPtr)

#define MAP_HASH_FUNC(param1, param2) MAP_HASH_FUNC_NAME() (param1, param2)
#define MAP_KEY_EQUAL_FUNC(param1, param2) MAP_KEY_EQUAL_FUNC_NAME() (param1, param2)
#define MAP_GET_OCCUPIED_FUNC(param) MAP_GET_OCCUPIED_FUNC_NAME() (param)
#define MAP_MARK_OCCUPIED_FUNC(param1, param2) MAP_MARK_OCCUPIED_FUNC_NAME() (param1, param2)
#define MAP_GET_KEY_PTR_FUNC(param1, param2) MAP_GET_KEY_PTR_FUNC_NAME() (param1, param2)

#define MAP_META(entryType) \
    ((MapMeta) { \
        sizeof(entryType), \
        (HashFunc) MAP_HASH_FUNC_NAME(), \
        (KeyEqualFunc) MAP_KEY_EQUAL_FUNC_NAME(), \
        (GetOccupiedFunc) MAP_GET_OCCUPIED_FUNC_NAME(), \
        (MarkOccupiedFunc) MAP_MARK_OCCUPIED_FUNC_NAME(), \
        (GetKeyPtrFunc) MAP_GET_KEY_PTR_FUNC_NAME() \
    })

#define M_PASS_0(k) (void*) k
#define M_PASS_1(k) (void*) &k

#define M_PASS_X(b, k) M_PASS_ ## b (k)

#define MAP_DECLARE_FUNCTIONS_STATIC(funcPrefix, entryType, keyType, useIndirection) \
    static void funcPrefix ## Init (CURRENT_MAP_TYPE()* map, uint32_t initialCapacity, float loadFactor) \
    { \
        mapInit((Map*) map, initialCapacity, loadFactor, MAP_META(entryType)); \
    } \
    \
    static entryType* funcPrefix ## Lookup (CURRENT_MAP_TYPE()* map, keyType key) \
    { \
        return (entryType*) mapLookup((Map*) map, M_PASS_X(useIndirection, key), \
                                      MAP_META(entryType)); \
    } \
    \
    static entryType* funcPrefix ## Insert (CURRENT_MAP_TYPE()* map, keyType key) \
    { \
        return (entryType*) mapInsert((Map*) map, M_PASS_X(useIndirection, key), \
                                      MAP_META(entryType)); \
    } \
    static void funcPrefix ## Free (CURRENT_MAP_TYPE()* map) \
    { \
        free(map->entries); \
    }\
    static void funcPrefix ## Clear (CURRENT_MAP_TYPE()* map, int32_t newCapacity) \
    { \
        mapClear((Map*) map, newCapacity, MAP_META(entryType)); \
    }
#else
#warning "map.h is only available in EXPERIMENTAL_ALGO mode!"
#endif
#endif //MAP_H
