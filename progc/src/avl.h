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

// The base struct for AVL trees.
// For making AVL trees of a type T, you need to either:
// - create another struct with the AVL struct as the first member.
// - replicate the exact same layout as this AVL struct (use the AVL_HEADER macro).
//   (recommended because it doesn't change the type to AVL* but to T* instead)
// Example 1:
// struct IntAVL { AVL avl; int value; };
// Example 2:
// struct IntAVL { struct IntAVL* left, right; int balance; int value; };
typedef struct AVL
{
    AVL_HEADER(AVL)
    // Expands to:
    // struct AVL *left, *right;
    // int balance;
} AVL;

// Returns | <= 1  when a.value > b
//         | 0     when a.value = b
//         | >= -1 when a.value < b
typedef int (*AVLCompareValueFunc)(const AVL* a, const void* b);

// Allocates an AVL node with the given value.
typedef AVL* (*AVLCreateFunc)(void* value);

typedef struct
{
    AVLCompareValueFunc compare;
    AVLCreateFunc create;
} AVLFuncs;

AVL* avlRotateLeft(AVL* a);

AVL* avlRotateRight(AVL* a);

AVL* avlDoubleRotateRight(AVL* tree);

AVL* avlDoubleRotateLeft(AVL* tree);

AVL* avlBalance(AVL* tree);

AVL* avlInsert(AVL* tree, void* value, AVLCreateFunc create, AVLCompareValueFunc compare, AVL** insertedNode,
               bool* alreadyPresent);

AVL* avlLookup(AVL* tree, const void* value, AVLCompareValueFunc compare);

// TODO: Delete functions? Sounds like we don't need those in our programs. Maybe I'm wrong...

#endif //AVL_H
