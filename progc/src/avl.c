#include "avl.h"

#include "stdlib.h"
#include "assert.h"

// Who invented those macros?? WHOOO??!
#undef max
#undef min

int max(const int a, const int b) { return a > b ? a : b; }
int max3(const int a, const int b, const int c) { return max(max(a, b), c); }
int min(const int a, const int b) { return a < b ? a : b; }
int min3(const int a, const int b, const int c) { return min(min(a, b), c); }

AVL* avlRotateLeft(AVL* a)
{
    //   a                    b     |
    //  / \                  / \    |
    // s   b       ->       a   c   |
    //    / \              / \      |
    //   t   c            s   t     | <---- Important change [t]

    assert(a && a->right);

    AVL* b = a->right;
    AVL* c = b->right;
    AVL* t = b->left; // may be NULL
    // AVL* s = a->left; // may be NULL

    b->right = c;
    b->left = a;
    a->right = t; // NULL instead of b if no t

    int aBal = a->balance;
    a->balance = aBal - max(b->balance, 0) - 1;
    b->balance = min3(aBal - 2, aBal + b->balance - 2, b->balance - 1);

    return b;
}

AVL* avlRotateRight(AVL* a)
{
    //     a                 b        |
    //    / \               / \       |
    //   b   s     ->      c   a      |
    //  / \                   / \     |
    // c   t                 t   s    |

    assert(a && a->left);
    AVL* b = a->left;
    AVL* c = b->left;
    AVL* t = b->right;

    b->right = a;
    b->left = c;
    a->left = t; // NULL with no t

    int aBal = a->balance;
    a->balance = aBal - min(b->balance, 0) + 1;
    b->balance = max3(aBal + 2, aBal + b->balance + 2, b->balance + 1);

    return b;
}

AVL* avlDoubleRotateRight(AVL* arbre)
{
    arbre->left = avlRotateLeft(arbre->left);
    return avlRotateRight(arbre);
}

AVL* avlDoubleRotateLeft(AVL* arbre)
{
    arbre->right = avlRotateRight(arbre->right);
    return avlRotateLeft(arbre);
}

AVL* avlBalance(AVL* arbre)
{
    if (arbre->balance >= 2)
    {
        if (arbre->right->balance <= -1)
        {
            return avlDoubleRotateLeft(arbre);
        }
        else
        {
            return avlRotateLeft(arbre);
        }
    }
    else if (arbre->balance <= -2)
    {
        if (arbre->left->balance >= 1)
        {
            return avlDoubleRotateRight(arbre);
        }
        else
        {
            return avlRotateRight(arbre);
        }
    }
    else
    {
        return arbre;
    }
}

AVL* avlInsertInternal(AVL* tree, void* value, AVLCreateFunc create, const AVLCompareValueFunc compare,
                       AVL** existingNode, int* h)
{
    assert(h);

    if (!tree)
    {
        *h = 1;
        return create(value);
    }

    int compareResult = compare(tree, value);
    if (compareResult == 0)
    {
        if (existingNode)
        {
            *existingNode = tree;
        }
    }
    else if (compareResult <= -1) // then parent < child ==> child > parent
    {
        tree->right = avlInsertInternal(tree->right, value, create, compare, existingNode, h);
    }
    else // then parent > child ==> child < parent
    {
        assert(compareResult >= 1);
        tree->left = avlInsertInternal(tree->left, value, create, compare, existingNode, h);
        *h = -*h;
    }

    if (*h != 0)
    {
        tree->balance += *h;
        tree = avlBalance(tree);
        if (tree->balance == 0)
        {
            // That means the height of tree didn't change.
            *h = 0;
        }
        else
        {
            // Reset the sign.
            *h = 1;
        }
    }

    return tree;
}

AVL* avlInsert(AVL* tree, void* value, const AVLCreateFunc create, const AVLCompareValueFunc compare,
               AVL** existingNode)
{
    int h = 0;
    return avlInsertInternal(tree, value, create, compare, existingNode, &h);
}

AVL* avlLookup(AVL* tree, void* value, const AVLCompareValueFunc compare)
{
    if (!tree)
    {
        return NULL;
    }

    int compareResult = compare(tree, value);
    if (compareResult == 0)
    {
        return tree;
    }
    else if (compareResult <= -1) // then tree < value ==> value > tree
    {
        return avlLookup(tree->right, value, compare);
    }
    else // then tree > value ==> value < tree
    {
        assert(compareResult >= 1);
        return avlLookup(tree->left, value, compare);
    }
}