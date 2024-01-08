#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct Dist {
  AVL_HEADER(Dist)
  float d;
  int m; //(multiplicateur)
} Dist;

typedef struct Travel {
  int ID;
  float min;
  float max;
  float moy;
  Dist *parc;
} Travel;

typedef struct TownS {
  AVL_HEADER(TownS)
  Travel t;
} TownS;

typedef struct AVLtempo {
  AVL_HEADER(AVLtempo)
  Travel *t;
} AVLtempo;

Dist *Distcreate(float* parc) {
  Dist *A = malloc(sizeof(Dist)); // création du chainon de l'AVL
  assert(A);
  A->d = *parc;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int distcompare(Dist *p, float* i) {
  if (*i > p->d) {
    return -4;
  }
  if (*i < p->d) {
    return 1;
  } else {
    return 0;
  }
}

int distsearch(Dist *p, float i) {
  if (p->d > i) {
    if (p->left != NULL) {
      return distsearch(p->left, i);
    } else {
      return 0;
    }
  }
  if (p->d < i) {
    if (p->right != NULL) {
      return distsearch(p->right, i);
    } else {
      return 0;
    }
  } else {
    return 1;
  }
}

TownS *TownScreate(Travel *T) {
  TownS *A = malloc(sizeof(TownS)); // création du chainon de l'AVL
  assert(A);
  A->t = *T;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int Townscompare(TownS *T, Travel *i) {
  if (T != NULL) {
    if (i->ID > T->t.ID) {
      return -4;
    }
    if (i->ID < T->t.ID) {
      return 1;
    } else {
      return 0;
    }
  }
}

void Max(Travel *T, Dist *d) {
  if (d->right != NULL) {
    Max(T, d->right);
  } else {
    T->max = d->d;
  }
}

void Min(Travel *T, Dist *d) {
  if (d->left != NULL) {
    Min(T, d->left);
  } else {
    T->min = d->d;
  }
}

void Moy(Travel *T, Dist *d, float *x, int *i) {
  if (d->right != NULL) {
    Moy(T, d->right, x, i);
  }
  if (d != NULL) {
    *x += d->d * d->m;
    *i += 1;
  }
  if (d->left != NULL) {
    Moy(T, d->left, x, i);
  }
}

  void Moyend(Travel *T, Dist *d, float *x, int *i) {
    Moy(T, d, x, i);
  T->moy = *x / *i;
}

  void maxminmoy(TownS *T) {

  if (T->left != NULL) {
    maxminmoy(T->left);
  }
  if (T != NULL) {
    float x=0;
    int i=0;
    Max(&T->t, T->t.parc);
    Min(&T->t, T->t.parc);
    Moyend(&T->t, T->t.parc, &x, &i);
  }
  if (T->right != NULL) {
    maxminmoy(T->right);
  }
}

AVLtempo *AVLcreate(Travel *T) {
  AVLtempo *A = malloc(sizeof(AVLtempo)); // création du chainon de l'AVL
  assert(A);
  A->t = T;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int AVLcompare(AVLtempo *A, Travel *T) {
  if (T->max - T->min > A->t->max - A->t->min) {
    return -4;
  }
  if (T->max - T->min < A->t->max - A->t->min) {
    return 1;
  } else {
    if (T->ID > A->t->ID) {
      return -4;
    }
    if (T->ID < A->t->ID) {
      return 1;
    }
  }
}

TownS *supTown2(TownS *f, Travel *passage) {
  if (f == NULL) // quand la chaine est vide
  {
    printf("No town!\n");
    return (NULL);
  } else {
    if (f != NULL && f->right != NULL) {
      f->right =supTown2(f->right, passage); // si il existe fils droit alors on cherche à suprrimer le fils droit
      return (f);
    }

    if (f->left != NULL) {
      f->left = supTown2(f->left,passage); // s'il n'existe pas de fils droit mais qu'il existe un fils gauche alors on voir pour supprimer le fils gauche
      return (f);
    }

    else {
      *passage = f->t; // si aucun fils n'existe: suppression du premier chainon (pour l'envoyer dans l'AVL)                  
      f->t = (Travel){0};
      free(f);
      return NULL;
    }
  }
}

void parcoursInf(AVLtempo *A, int *x) {
  if ((A->right != NULL && *x < 50) && A != NULL) {
    parcoursInf(A->right, x);
  }
  if (*x < 50) {
  (*x) += 1;
  printf("%d;%d;%f;%f;%f;%f\n",*x, A->t->ID,A->t->min,A->t->moy, A->t->max, A->t->max-A->t->min);
    
  }
  if (A != NULL && (A->left != NULL && *x < 50)) {
    parcoursInf(A->left, x);
  }
}

void creationInf(TownS *A, AVLtempo **C) {
  if (A->right!=NULL) {
    creationInf(A->right, C);
  }
    *C = avlInsert(*C, &A->t, &AVLcreate, &AVLcompare, NULL, NULL);

  if (A->left != NULL ) {
    creationInf(A->left, C);
  }
}

void freepostfixe(AVL* T){
  if(T->left!=NULL){
    freepostfixe(T->left);
  }
  if(T->right!=NULL){
    freepostfixe(T->right);
  }
  free(T);
}

void computationS(RouteStream *stream) {
  TownS *T=NULL;
  TownS* F;
  Travel t;
  AVLtempo *A=NULL;
  int x = 0;
  RouteStep step;
  while (rsRead(stream, &step, ALL_FIELDS & ~TOWN_A & ~TOWN_B & ~STEP_ID)) {
    t.ID = step.routeId;
    F=avlLookup(T,  &t, Townscompare);
    if(F==NULL){
    t.parc = Distcreate(&step.distance);
    t.parc->m=1;
    T = avlInsert(T, &t, TownScreate, Townscompare, NULL, NULL);
    }
    else{
      Dist* g;
      bool b;
      F->t.parc=avlInsert(F->t.parc, &step.distance, Distcreate, distcompare, &g, &b);
      if(b){
        g->m+=1;
      }
      else{
        g->m=1;
      }
    }
  }
  x = 0;
  maxminmoy(T);
  creationInf(T, &A);
  parcoursInf(A, &x);
  freepostfixe(A);
  freepostfixe(T);
}