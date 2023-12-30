#include "computations.h"
#include "route.h"
#include <string.h>
#include <stdlib.h>
#include "avl.h"
#include <assert.h>

typedef struct Driver
{
    char name[100];
    struct Driver *next;
} Driver;

typedef struct ID
{
    AVL_HEADER(ID);
    int iD;
} ID;

typedef struct Town
{
    char name[100];
    struct Town *next;
    int passed; // compteur
    ID *IDhead; // chaine d'ID
    Driver *head; // chaine de chauffeurs
    Driver *tail;
} Town;

typedef struct AVL1
{
    AVL_HEADER(AVL1);
    Town value;
}AVL1;


typedef struct File
{
    struct Town *head;
    struct Town *tail;
} File;

typedef struct AVLT
{
    AVL_HEADER(AVLT);
    Town value;
} AVLT;

AVLT *IDCreate(int i)
{
    ID *A = malloc(sizeof(ID));
    assert(A);
    A->iD = i;
    A->left = NULL;
    A->right = NULL;
    A->balance = 0;
    return A;
}

int IDCompare(ID *A, int i)
{
    if (i > A->iD)
    {
        return -4;
    }
    if (i < A->iD)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int searchID(ID *h, int Id)
{
    ID *p1 = h;
    while (p1 != NULL)
    {
        if (p1->iD == Id) // Si l'ID existe dans la chaine
        {
            return 1;
        }
        else
        {
            if(p1->iD<Id){
                 if(p1->left!=NULL){
                    return searchID(p1->left, Id);
                 }
                 else{
                    return 0;
                 }
            }
            if(p1->iD>Id){
                 if(p1->right!=NULL){
                    return searchID(p1->right, Id);
                 }
                 else{
                    return 0;
                 }
            }
        }
    return 0; // sinon
    }
}

/*
File *insertTown(File *f, Town *T, int ID)
{
    if (f == NULL) // quand la file est vide: création du premier chainon
    {
        f = malloc(sizeof(File));
        T->IDhead = malloc(sizeof(ID));
        T->IDhead->left = NULL;
        T->IDhead->right = NULL;
        T->IDhead->iD = ID;
        T->passed=1;
        T->next=NULL;
        f->head = T;
        f->tail = T;
        return f;
    }
    else
    {
        Town *p1 = f->head;
        while (p1 != NULL) // sinon
        {
            if (strcmp(p1->name, T->name) == 0)
            {
                if (searchID(p1->IDhead, ID) == 1)
                { // quand la ville avec le meme ID a été trouvée dans la file

                    return f;
                }
                else
                {
                    p1->passed += 1;
                    if (T->IDhead != NULL)
                    {
                        p1->IDhead = avlInsert(p1->IDhead, ID, &IDCreate, &IDCompare, NULL, NULL); // ajout de l'ID dans la chaine d'ID de la ville avec AVL
                    }
                    return f;
                }
            }
            p1 = p1->next;
        }
        T->IDhead = avlInsert(T->IDhead, ID, &IDCreate, &IDCompare, NULL, NULL);
        T->passed=1;
        T->next=NULL;
        f->tail->next = T;
        f->tail = T;

        // driver à ajouter
        return f;
    }
 }*/

AVL1 *supTown(AVL1 *f, Town *passage)
{
    if (f->value.name == NULL) // quand la chaine est vide
    {
        printf("No town!\n");
        return (NULL);
    }
    else
    {
        if(f->right!=NULL){
            f->right=supTown(f->right, &passage);
            return(f);
        }

        if(f->left!=NULL){
            f->left=supTown(f->left, &passage);
            return(f);
        }

        else{
            *passage = f->value; // sinon suppression du premier chainon (pour l'envoyer dans l'AVL)
            f->value=(Town){0};
            free(f);
            return NULL;
        }
        return f;
    }
}

AVL1 *AVL1Create(Town *T)
{
    AVL1 *A = malloc(sizeof(AVL1));
    assert(A);
    A->value = *T;
    A->left = NULL;
    A->right = NULL;
    A->balance = 0;
    return A;
}

int AVL1Compare(AVLT *A, Town *T)//Id est l'Id de la ville à ajouter dans l'AVL
{
    if (strcmp(A->value.name, T->name)<0)
    {
        return -4;
    }
    if (strcmp(A->value.name, T->name)>0)
    {
        return 1;
    }
    else
    {
        if(searchID(A->value.IDhead, T->IDhead->iD)){
            A->value.passed+=1;
        }
        return 0;
    }
}

AVLT *AVLCreate(Town *T)
{
    AVLT *A = malloc(sizeof(AVLT));
    assert(A);
    A->value = *T;
    A->left = NULL;
    A->right = NULL;
    A->balance = 0;
    return A;
}

int AVLCompare(AVLT *A, Town *T)
{
    if (T->passed > A->value.passed)
    {
        return -4;
    }
    if (T->passed < A->value.passed)
    {
        return 1;
    }
    else
    {
        return strcmp(A->value.name, T->name);
    }
}

int parcoursInfInv(AVLT *A, int x)
{
    if (A->right != NULL && x < 10)
    {
        x += parcoursInfInv(A->right,x);
    }
    if (x < 10)
    {
        printf("%s, %d\n", A->value.name, A->value.passed);
        x += 1;
    }
    if (A->left != NULL && x < 10)
    {
        x += parcoursInfInv(A->left,x);
    }
    return x;
}

void computationT(RouteStream *stream)
{
    int i = 0;
    Town* T;
    AVL1 *f = malloc(sizeof(File));
    AVLT *A;
    A = NULL;
    f = NULL;
    RouteStep step;
    while (rsRead(stream, &step, ALL_FIELDS & ~DISTANCE))
    {
        T = malloc(sizeof(Town));
        strcpy(T->name, step.townA);
        T->IDhead=malloc(sizeof(ID));
        T->IDhead->iD=step.routeId;
        f = f = avlInsert(f, T, &AVL1Create, &AVL1Compare, NULL, NULL);
        T = malloc(sizeof(Town));
        T->IDhead=malloc(sizeof(ID));
        strcpy(T->name, step.townB);
        T->IDhead->iD=step.routeId;
        f = avlInsert(f, T, &AVL1Create, &AVL1Compare, NULL, NULL);
        i += 1;
        if (i== 100)
            break;
    }

    while (f!=NULL)
{
    f = supTown(f, T);
    A = avlInsert(A, T, &AVLCreate, &AVLCompare, NULL, NULL);
    printf("%s\n", T->name);
}
    free(f);
    parcoursInfInv(A,0);
}