#ifndef PARTITION_H
#define PARTITION_H

/*
 * partition.h
 * -----------------------
 * The partitioner is a data structure that stores route steps -- copied from the file --
 * in multiple partitions. Items with the same key (here, the route id) are stored in the same partition.
 *
 * Partitions can be iterated, guaranteeing that all items with the same key are iterated within a single partition.
 *
 * Partioning allows for great performance benefits!
 *  - Iterating partitions instead of the file grants much better CPU cache efficiency, by accessing
 *    the same memory locations more often.
 *  - Knowing that we've seen each item of the same key (e.g. steps with the same route id) allows us
 *    to discard those items in some computations, as we're done with them.
 *
 * Usage of the partitioner is usually done in two steps:
 *  1. Read the file (with RouteStream), and add all items to the partitioner, with partitionerAddS.
 *  2. Iterate through all the partitions (with PARTITIONER_ITERATE), and run any algorithm on the items.
 *
 * That first step induces a slight overhead, and requires a lot of memory,
 * as all items need to be copied from the file, although items are usually stripped down.
 * However, the second step is way faster, which compensates the earlier overhead.
 *
 * Ultimately, the partitioner works like a small hash map of huge contiguous lists.
 * Simple, but remarkably efficient!
 *
 * Only available with EXPERIMENTAL_ALGO for obvious reasons.
 */

#include "compile_settings.h"
#if EXPERIMENTAL_ALGO

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

typedef struct PartDataList
{
    struct PartDataList* next;
    uint8_t data[];
} PartDataList;

typedef struct Partition
{
    uint8_t* tailCursor;
    uint8_t* tailEnd;

    PartDataList* tail;
    PartDataList* head;
} Partition;

typedef struct Partitioner
{
    Partition* partitions;
    uint32_t numPartitions;
    uint32_t partitionSize;

    uint32_t numSteps;
} Partitioner;

static void partitionerInit(Partitioner* partitioner, uint32_t numPartitions, uint32_t partitionSize)
{
    assert((numPartitions & (numPartitions - 1)) == 0 && "Num partitions must be a power of 2!");

    partitioner->partitions = malloc(sizeof(Partition) * numPartitions);
    assert(partitioner->partitions);

    partitioner->numPartitions = numPartitions;
    partitioner->partitionSize = partitionSize;

    partitioner->numSteps = 0;

    for (uint32_t i = 0; i < numPartitions; ++i)
    {
        PartDataList* list = malloc(sizeof(PartDataList) + partitionSize);
        partitioner->partitions[i].head = list;
        partitioner->partitions[i].tail = list;
        partitioner->partitions[i].tailCursor = list->data;
        partitioner->partitions[i].tailEnd = list->data + partitionSize;
    }
}

static void partinitionerAdd(Partitioner* partitioner, uint32_t key, void* element, uint32_t elementSize)
{
    Partition* part = &partitioner->partitions[key & (partitioner->numPartitions - 1)];

    partitioner->numSteps++;

    if (part->tailCursor + elementSize <= part->tailEnd)
    {
        memcpy(part->tailCursor, element, elementSize);
        part->tailCursor += elementSize;
    }
    else
    {
        PartDataList* list = malloc(sizeof(PartDataList) + partitioner->partitionSize);
        list->next = NULL;

        part->tail->next = list;
        part->tail = list;
        part->tailCursor = list->data + elementSize;
        part->tailEnd = list->data + partitioner->partitionSize;

        memcpy(list->data, element, elementSize);
    }
}

static void partitionerFree(Partitioner* partitioner)
{
    if (partitioner == NULL || partitioner->partitions == NULL)
    {
        return;
    }

    for (int i = 0; i < partitioner->numPartitions; ++i)
    {
        Partition* partition = &partitioner->partitions[i];
        PartDataList* list = partition->head;
        while (list)
        {
            PartDataList* temp = list;
            list = list->next;
            free(temp);
        }
    }

    free(partitioner->partitions);
}

#define partinitionerAddS(partitioner, key, element) \
    partinitionerAdd(partitioner, key, &element, sizeof(element))

/*
 * Partition iteration
 */

static void* partItNextLimit(Partitioner* partitioner, Partition* partition, PartDataList* list, size_t elSize)
{
    uint8_t* addr = list != partition->tail ? list->data + partitioner->partitionSize : partition->tailCursor;
    if (partitioner->partitionSize % elSize != 0)
    {
        addr = addr - (addr - list->data) % elSize;
    }
    return addr;
}

#define PARTITION_ITERATE(partitioner, part, type, var) \
for (PartDataList* _pi_l = part->head; _pi_l != NULL; _pi_l = _pi_l->next)\
for (type *var = (type*) _pi_l->data, *limit = partItNextLimit(partitioner, part, _pi_l, sizeof(type)); var < limit; var++)

#define PARTITIONER_ITERATE(partitioner, type, var) \
for (Partition* _pi_p = (partitioner)->partitions; _pi_p != (partitioner)->partitions + (partitioner)->numPartitions; ++_pi_p)\
PARTITION_ITERATE(partitioner, _pi_p, type, var)
#else
#warning "partition.h can only be used if EXPERIMENTAL_ALGO is on!"
#endif
#endif //PARTITION_H
