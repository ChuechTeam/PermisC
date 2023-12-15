#include "string_avl.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int stringAVLCompare(const StringAVL* tree, const void* strPtr)
{
    return strcmp(tree->value, strPtr);
}

StringAVL* stringAVLCreate(char* value)
{
    assert(value);
    StringAVL* node = malloc(sizeof(StringAVL));
    node->value = value;
    node->balance = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void stringAVLDelete(StringAVL* node)
{
    if (node)
    {
        free(node->value);
        stringAVLDelete(node->left);
        stringAVLDelete(node->right);
        free(node);
    }
}

static AVLFuncs funcs = { (AVLCompareValueFunc)&stringAVLCompare, (AVLCreateFunc)&stringAVLCreate };

StringAVL* stringAVLInsert(StringAVL* tree, char* value, StringAVL** existingNode)
{
    return (StringAVL*) avlInsert((AVL*)tree, value, funcs.create, funcs.compare, (AVL**)existingNode);
}
