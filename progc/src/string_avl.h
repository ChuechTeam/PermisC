#ifndef STRING_AVL_H
#define STRING_AVL_H

/*
 * string_avl.h
 * ---------------
 * An AVL tree of strings, allocated using malloc.
 */

#include "avl.h"

typedef struct StringAVL
{
    AVL_HEADER(StringAVL)

    char* value;
} StringAVL;

// Allocates an AVL node with a malloc'd string.
StringAVL* stringAVLCreate(char* value);

void stringAVLDelete(StringAVL* node);

StringAVL* stringAVLInsert(StringAVL* tree, char* value, StringAVL** existingNode);

#endif //STRING_AVL_H
