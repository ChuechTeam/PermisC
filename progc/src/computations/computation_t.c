#include "computations.h"
#include "route.h"

typedef struct Driver{
    char name[100];
    struct Driver* next;
}Driver;

typedef struct Town{
    char name[100];
    int passed; //compteur
    struct Town* next;
    Driver* head;
    Driver* tail;
}Town;

typedef struct File{
    struct Town* head;
    struct Town* tail;
}File;

Town* NewTown(char* townName){
    Town* c= malloc(sizeof(Town));
    if(c==NULL){
        return 0;
    }
    c->name=townName;
    c->next=NULL;
}

File *insertTown(File* f, Town* T){
    if(f->head==NULL){
        f->head=T;
        f->tail=T;
    }
    else{
        f->tail->next=T;
        f->tail=T;
    }
    return f;
}
File* supTown(File* f, int* passage){
    Town* p1=f->head;
    if(f->head==NULL){
        printf("No town!\n");
        return(NULL);
    }
    else{
        *passage= f->head->passed;
        f->head=f->head->next;
        free(p1);
        return f;
    }
}

void computationT(RouteStream* stream)
{
    Driver* D
    Town* T;
    File* f;

    RouteStep step;
    while (rsRead(&stream, &step, ALL_FIELDS & ~DISTANCE))
        {
            if(step.townA);
        }
}
/* récuptown A, town B 
récup le step ID
si pas déjà utilisé:
chainon de ville avec chainon de chauffeurs pour chaque ville

Plus tard AVL aussi...