#include "avl.h"

#include <stdio.h>

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

AVL* avlDoubleRotateRight(AVL* tree)
{
    tree->left = avlRotateLeft(tree->left);
    return avlRotateRight(tree);
}

AVL* avlDoubleRotateLeft(AVL* tree)
{
    tree->right = avlRotateRight(tree->right);
    return avlRotateLeft(tree);
}

AVL* avlBalance(AVL* tree)
{
    if (tree->balance >= 2)
    {
        if (tree->right->balance <= -1)
        {
            return avlDoubleRotateLeft(tree);
        }
        else
        {
            return avlRotateLeft(tree);
        }
    }
    else if (tree->balance <= -2)
    {
        if (tree->left->balance >= 1)
        {
            return avlDoubleRotateRight(tree);
        }
        else
        {
            return avlRotateRight(tree);
        }
    }
    else
    {
        return tree;
    }
}

// I kept the old version of the insert function just for reference.
/*
AVL* avlInsertInternal(AVL* tree, void* value, AVLCreateFunc create, const AVLCompareValueFunc compare,
                       AVL** insertedNode, bool* alreadyPresent, int* h)
{
    assert(h);

    if (!tree)
    {
        *h = 1;
        AVL* newNode = create(value);
        if (insertedNode)
        {
            *insertedNode = newNode;
        }
        if (alreadyPresent)
        {
            *alreadyPresent = false;
        }
        return newNode;
    }

    int compareResult = compare(tree, value);
    if (compareResult == 0)
    {
        if (insertedNode)
        {
            *insertedNode = tree;
        }
        if (alreadyPresent)
        {
            *alreadyPresent = true;
        }
    }
    else if (compareResult <= -1) // then parent < child ==> child > parent
    {
        tree->right = avlInsertInternal(tree->right, value, create, compare, insertedNode, alreadyPresent, h);
    }
    else // then parent > child ==> child < parent
    {
        tree->left = avlInsertInternal(tree->left, value, create, compare, insertedNode, alreadyPresent, h);
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
*/

// The iterative version of AVL insert. Performs a bit faster than the recursive one,
// especially when the element is already in the tree (which is VERY common).
AVL* avlInsert(AVL* tree, void* value, const AVLCreateFunc create, const AVLCompareValueFunc compare,
               AVL** insertedNode, bool* alreadyPresent)
{
    struct AVLDiff
    {
        AVL* node;
        int dir; // -1 for left, 1 for right
    };

    // 2^64 nodes should be enough right?
    struct AVLDiff stack[64];
    int depth = 0;

    AVL* subtree = tree;
    while (true)
    {
        if (subtree == NULL)
        {
            AVL* newNode = create(value);
            if (insertedNode)
            {
                *insertedNode = newNode;
            }
            if (alreadyPresent)
            {
                *alreadyPresent = false;
            }
            stack[depth] = (struct AVLDiff){newNode, 0};

            break;
        }

        int compareResult = compare(subtree, value);
        if (compareResult == 0)
        {
            if (insertedNode)
            {
                *insertedNode = subtree;
            }
            if (alreadyPresent)
            {
                *alreadyPresent = true;
            }

            // Return the tree directly, there will be no balancing changes as
            // we've pretty much inserted nothing.
            return tree;
        }
        else if (compareResult <= -1) // then parent < child ==> child > parent
        {
            stack[depth] = (struct AVLDiff){subtree, 1};
            subtree = subtree->right;
        }
        else // then parent > child ==> child < parent
        {
            stack[depth] = (struct AVLDiff){subtree, -1};
            subtree = subtree->left;
        }
        depth++;
        assert(depth != 64);
    }

    bool noMoreBalanceChanges = false;
    depth--; // Start with the parent of the last inserted node.
    while (depth >= 0)
    {
        AVL* me = stack[depth].node;
        int dir = stack[depth].dir;
        AVL* child = stack[depth + 1].node;

        // Step 1: Put the child in the right place.
        if (dir == 1)
        {
            me->right = child;
        }
        else
        {
            me->left = child;
        }

        // Step 1.5: Are we done balancing the tree? If so, let's take a break.
        if (noMoreBalanceChanges)
        {
            break;
        }

        // Step 2: Oh wow I got a new child! Adjust the balance accordingly.
        me->balance += dir;
        me = avlBalance(me); // This might require a change to the parent node we're going to do later.
        stack[depth].node = me;

        // If the balance becomes 0, then we have made both subtrees the same height,
        // so the tree didn't become taller.
        if (me->balance == 0)
        {
            noMoreBalanceChanges = true;
        }

        depth--;
    }

    return stack[0].node;
}

AVL* avlLookup(AVL* tree, const void* value, const AVLCompareValueFunc compare)
{
    while (tree != NULL)
    {
        int compareResult = compare(tree, value);
        if (compareResult == 0)
        {
            return tree;
        }
        else if (compareResult <= -1) // then tree < value ==> value > tree
        {
            tree = tree->right;
        }
        else // then tree > value ==> value < tree
        {
            tree = tree->left;
        }
    }

    return NULL;
}
