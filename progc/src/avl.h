#ifndef AVL_H
#define AVL_H

/*
 * avl.h
 * ---------------
 * Common functions for AVL trees of any type (string, float, int...)
 */

#include <stdbool.h>

// The common fields to all AVL trees. Must be placed at the very beginning of the struct.
#define AVL_HEADER(type) struct type *left, *right; int balance;

// Sets all AVL fields to zero values (left and right are NULL, balance is 0).
#define AVL_INIT(tree) (tree)->left = (tree)->right = NULL; (tree)->balance = 0;

// Creates a function for an AVL tree type that calls the avlInsert function, by applying the types correctly.
#define AVL_DECLARE_INSERT_FUNCTION(funcName, treeType, valueType, funcA, funcB) \
    treeType* funcName(treeType* tree, valueType* value, treeType** insertedNode, bool* alreadyPresent) \
    { \
        return (treeType*) avlInsert((AVL*) tree, (void*) value, (funcA), (funcB), (AVL**) insertedNode, alreadyPresent); \
    }

// Creates a function for an AVL tree type that calls the avlLookup function, by applying the types correctly.
#define AVL_DECLARE_LOOKUP_FUNCTION(funcName, treeType, valueType, compareFunc) \
    treeType* funcName(treeType* tree, valueType* value) \
    { \
        return (treeType*) avlLookup((AVL*) tree, (void*) value, (compareFunc)); \
    }

// Creates both the insert and lookup functions that are static (only valid in current .c file), all in one macro!
#define AVL_DECLARE_FUNCTIONS_STATIC(funcPrefix, treeType, valueType, createFunc, compareFunc) \
    static AVL_DECLARE_INSERT_FUNCTION(funcPrefix ## Insert, treeType, valueType, createFunc, compareFunc) \
    static AVL_DECLARE_LOOKUP_FUNCTION(funcPrefix ## Lookup, treeType, valueType, compareFunc)

// The base struct for AVL trees.
// For making AVL trees of a type T, you need to either:
// - create another struct with the AVL struct as the first member.
// - replicate the exact same layout as this AVL struct (use the AVL_HEADER macro).
//   (recommended because it doesn't change the type to AVL* but to T* instead)
// Example 1:
// struct IntAVL { AVL avl; int value; };
// Example 2:
// struct IntAVL { struct IntAVL* left, right; int balance; int value; };
// Example 3 (recommended):
// struct IntAVL { AVL_HEADER(IntAVL) int value; };
typedef struct AVL
{
    AVL_HEADER(AVL)
    // Expands to:
    // struct AVL *left, *right;
    // int balance;
} AVL;

// Returns | >= 1  when a.value > b
//         | 0     when a.value = b
//         | <= -1 when a.value < b
typedef int (*AVLCompareValueFunc)(const AVL* a, const void* b);

// Allocates an AVL node with the given value.
typedef AVL* (*AVLCreateFunc)(void* value);

// Inserts some value into the AVL. Returns the new root of the AVL tree.
AVL* avlInsert(AVL* tree, void* value, AVLCreateFunc create, AVLCompareValueFunc compare, AVL** insertedNode,
               bool* alreadyPresent);

// Search for a node with the given value in the AVL. Returns the found node.
AVL* avlLookup(AVL* tree, const void* value, AVLCompareValueFunc compare);

#endif //AVL_H
