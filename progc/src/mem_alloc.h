#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

/*
 * mem_alloc.h
 * ---------------
 * A basic memory arena allocator, with elements having custom alignment.
 * Allows allocation of elements in multiple contiguous memory regions, which can be
 * freed in a single function call.
 *
 * Functions are defined static for easier inlining, also because it's a small utility.
 *
 * By the way, this is considered experimental! So memAlloc can't be used in
 * non-experimental implementations.
 */

#include "compile_settings.h"

#if EXPERIMENTAL_ALGO

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

typedef struct MemBlock
{
    struct MemBlock* prev;

    uint8_t data[];
} MemBlock;

typedef struct MemArena
{
    MemBlock* block;

    size_t blockSize;
    size_t blockPos; // 8-byte aligned
    size_t alignmentMask;
} MemArena;

// Allocates a block of memory.
// Exits the program if the allocation fails.
static MemBlock* memBlockAlloc(MemBlock* prev, const size_t blockSize)
{
    MemBlock* block = malloc(sizeof(MemBlock) + blockSize);
    assert(block);

    block->prev = prev;
    return block;
}

// Initialize the memory arena allocator, with the given block size.
// Smaller block sizes will lead to less memory waste, but will trigger frequent allocations.
static void memInitEx(MemArena* arena, const size_t blockSize, const size_t alignment)
{
    assert(arena);
    assert(blockSize >= 8);
    assert(alignment > 0 && ((alignment & (alignment-1)) == 0));

    arena->block = memBlockAlloc(NULL, blockSize);
    arena->blockSize = blockSize;
    arena->blockPos = 0;
    arena->alignmentMask = alignment-1;
}

static void memInit(MemArena* arena, const size_t blockSize)
{
    memInitEx(arena, blockSize, 8);
}

// Allocate a block of memory in the arena allocator.
// The returned pointer will be aligned by 8 bytes, and may be present
// in another memory block if there's not enough room left.
// Exits the program if the allocation fails.
static void* memAlloc(MemArena* arena, size_t size)
{
    assert(arena);

    void* fitPtr = arena->block->data + arena->blockPos;

    // Apply alignment
    size = size + (size & arena->alignmentMask);
    size_t newPos = arena->blockPos + size;

    if (newPos < arena->blockSize)
    {
        arena->blockPos = newPos;

        return fitPtr;
    }
    else
    {
        // Make sure the element isn't bigger than the block itself.
        assert(size < arena->blockSize);

        arena->block = memBlockAlloc(arena->block, arena->blockSize);
        assert(arena->block);

        // Advance the position in advance as we've just allocated a new item.
        arena->blockPos = size;

        return arena->block->data;
    }
}

// Frees all the blocks allocated by the arena allocator.
static void memFree(MemArena* arena)
{
    assert(arena);

    MemBlock* it = arena->block;
    while (it)
    {
        MemBlock* prev = it->prev;
        free(it);
        it = prev;
    }

    arena->block = NULL;
    arena->blockPos = 0;
}

#else //EXPERIMENTAL_ALGO
#warning mem_alloc.h can only be included while EXPERIMENTAL_ALGO is enabled!
#endif //EXPERIMENTAL_ALGO
#endif //MEM_ALLOC_H