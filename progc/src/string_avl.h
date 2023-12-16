#ifndef STRING_AVL_H
#define STRING_AVL_H

/*
 * string_avl.h
 * ---------------
 * An AVL tree of strings, allocated a flexible array member.
 */

#include "avl.h"

typedef struct StringAVL
{
    AVL_HEADER(StringAVL)

    // Can contain any malloc'd data. Will be freed in stringAVLFree.
    void* extraData;
    // The string of the node.
    char value[];
} StringAVL;

// Allocates an AVL node by duplicating the given string.
StringAVL* stringAVLCreate(const char* value, void* extraData);

// Default algorithm to free an AVL tree. Also calls free on the extraData field.
// NOTE: Does *not* update the balance fields accordingly.
//       This should only be used to remove the root of the tree.
void stringAVLFree(StringAVL* node);

// Inserts a new element into the tree, and returns the new tree (it is possible that tree != returnValue).
// The insertedNode parameter will contain the inserted OR preexisting node.
//     (Can be set to NULL to ignore)
// The alreadyPresent parameter will be set to true if the value was already present in the tree.
//     (Can be set to NULL to ignore)
StringAVL* stringAVLInsert(StringAVL* tree, char* value, void* extraData, StringAVL** insertedNode,
                           bool* alreadyPresent);

// Looks up a node in the tree, matching the given string (value).
// Returns NULL if not found.
StringAVL* stringAVLLookup(StringAVL* tree, const char* value);

// Debug function for printing the tree in DOT format.
void stringAVLPrintDOT(StringAVL* a);

#endif //STRING_AVL_H
