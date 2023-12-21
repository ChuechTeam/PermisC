#include "string_avl.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Functions for interfacing with the generic AVL tree.
 */

typedef struct { char* value; void* extraData; } StringAVLCreateData;

int stringAVLCompareG(const AVL* tree, const void* createData)
{
    char* a = ((StringAVL*)tree)->value;
    char* b = ((StringAVLCreateData*)createData)->value;

    // Quickly eliminate some obviously non-equal strings.
    char diff = *a - *b;
    if (diff != 0)
    {
        return diff;
    }

    return strcmp(a, b);
}

AVL* stringAVLCreateG(void* createData)
{
    StringAVLCreateData* data = createData;
    return (AVL*)stringAVLCreate(data->value, data->extraData);
}

static AVLFuncs funcs = { &stringAVLCompareG, &stringAVLCreateG };

/*
 * Normal functions.
 */

StringAVL* stringAVLCreate(const char* value, void* extraData)
{
    assert(value);
    size_t len = strlen(value);
    StringAVL* node = malloc(sizeof(StringAVL) + len + 1);
    assert(node);

    memcpy(node->value, value, len + 1);
    node->length = (uint32_t) len;
    node->extraData = extraData;
    node->balance = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void stringAVLFree(StringAVL* node)
{
    if (node)
    {
        stringAVLFree(node->left);
        stringAVLFree(node->right);
        free(node->extraData);
        free(node);
    }
}

StringAVL* stringAVLInsert(StringAVL* tree, char* value, void* extraData, StringAVL** insertedNode, bool* alreadyPresent)
{
    StringAVLCreateData createData = { value, extraData };

    return (StringAVL*) avlInsert((AVL*)tree, &createData, funcs.create, funcs.compare, (AVL**)insertedNode, alreadyPresent);
}

StringAVL* stringAVLLookup(StringAVL* tree, const char* value)
{
    return (StringAVL*) avlLookup((AVL*)tree, value, funcs.compare);
}

void printDOTHeader(StringAVL* a, char dir)
{
    printf("\"%s\"[label=<%s(%d%c)>]; ", a->value, a->value, a->balance, dir);
}

void printDOT(StringAVL* a)
{
    if (!a)
    {
        return;
    }
    if (a->left)
    {
        printDOTHeader(a->left, 'l');
        printf("\"%s\" -> \"%s\"; ", a->value, a->left->value);
        printDOT(a->left);
    }
    if (a->right)
    {
        printDOTHeader(a->right, 'r');
        printf("\"%s\" -> \"%s\"; ", a->value, a->right->value);
        printDOT(a->right);
    }
}

void stringAVLPrintDOT(StringAVL* a)
{
    if (!a)
    {
        return;
    }
    printDOTHeader(a, 'r');
    printDOT(a);
    printf("\n");
}
