#include "computations.h"
#include "route.h"
#include <string.h>
#include <stdlib.h>
typedef struct Driver
{
    char name[100];
    struct Driver *next;
} Driver;

typedef struct ID
{
    int ID;
    struct ID *next;
} ID;

typedef struct Town
{
    char name[100];
    struct Town *next;
    int passed; // compteur
    ID *IDhead; // chaine d'ID
    ID *IDtail;
    Driver *head; // chaine de chauffeurs
    Driver *tail;
} Town;

typedef struct File
{
    struct Town *head;
    struct Town *tail;
} File;

int searchID(ID *h, int Id)
{
    ID* p1 = h;
    while (p1 != NULL)
    {
        if (p1->ID == Id) // Si l'ID existe dans la chaine
        {
            return 1;
        }
        else
        {
            p1 = p1->next;
        }
    }
    return 0; // sinon
}

File *insertTown(File *f, Town *T, ID *ID)
{
    Town *p1 = f->head;
    if (f->head == NULL) // quand la file est vide: création du premier chainon
    {
        f->head = T;
        f->tail = T;
    }
    else
    {
        while (p1 != NULL)
        {
            if (strcmp(p1->name, T->name) == 0)
            {
                if (searchID(p1->IDhead, ID) == 1)
                { // quand la ville avec le meme ID a été trouvée dans la file
                    p1->passed += 1;
                    return f;
                }
                else
                {
                    p1 = p1->next;
                }
            }
            f->tail->next = T;
            f->tail = T;
            T->IDtail->next = ID; // ajout de l'ID dans la chaina d'ID de la ville
            T->IDtail->next = ID;
            // driver à ajouter
            return f;
        }
    }
}

File *supTown(File *f, Town *passage)
{
    Town *p1 = f->head;
    if (f->head == NULL) // quand la chaine est vide
    {
        printf("No town!\n");
        return (NULL);
    }
    else
    {
        *passage = *f->head; // sinon suppression du premier chainon (pour l'envoyer dans l'AVL)
        f->head = f->head->next;
        free(p1);
        return f;
    }
}

void computationT(RouteStream *stream)
{
    Town *T= malloc(sizeof(Town));
    File *f = malloc(sizeof(File));
    f = NULL;
    RouteStep step;
    while (rsRead(stream, &step, ALL_FIELDS & ~DISTANCE))
    {   
        strcpy(T->name,step.townA);
        f = insertTown(f, T, step.routeId);
        strcpy(T->name,step.townB);
        f = insertTown(f, T, step.routeId);
    }
}
/* récuptown A, town B :fait
récup le step ID: fait
si pas déjà utilisé:
chainon de ville avec chainon de chauffeurs et chainon d'ID pour chaque ville :fait

Plus tard AVL aussi...*/