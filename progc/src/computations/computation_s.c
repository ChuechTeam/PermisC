#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct Dist {
  AVL_HEADER(Dist);
  float d;
} Dist;

typedef struct Travel {
  int ID;
  float min;
  float max;
  float moy;
  Dist *parc;
} Travel;

typedef struct TownS {
  AVL_HEADER(TownS);
  Travel *t;
} TownS;

typedef struct AVLtempo {
  AVL_HEADER(AVLtempo);
  Travel *t;
} AVLtempo;

Dist *Distcreate(float parc) {
  Dist *A = malloc(sizeof(Dist)); // création du chainon de l'AVL
  assert(A);
  A->d = parc;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int distcompare(Dist *p, float i) {
  if (i > p->d) {
    return -4;
  }
  if (i < p->d) {
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
  A->t = T;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int Townscompare(TownS *T, Travel *i) {
  if (T->t != NULL) {
    if (i->ID > T->t->ID) {
      return -4;
    }
    if (i->ID < T->t->ID) {
      return 1;
    } else {
      if (distsearch(T->t->parc, i->parc->d) == 0) {
        T->t->parc = avlInsert(T->t->parc, &i->parc->d, &Distcreate, distcompare,
                         NULL, NULL);
      }
      return 0;
    }
  }
  else{
    T->t=i;
  }
}

Travel *Max(Travel *T, Dist *d) {
  if (d->right != NULL) {
    T = Max(T, d->right);
    return T;
  } else {
    T->max = d->d;
    return T;
  }
}

Travel *Min(Travel *T, Dist *d) {
  if (d->left != NULL) {
    T = Min(T, d->left);
    return T;
  } else {
    T->min = d->d;
    return T;
  }
}

Travel *Moy(Travel *T, Dist *d, float *x, int *i) {
  if (d->right != NULL) {
    T = Moy(T, d->right, x, i);
    return T;
  }
  if (d != NULL) {
    *x += d->d;
    *i += 1;
    return T;
  }
  if (d->left != NULL) {
    T = Moy(T, d->left, x, i);
    return T;
  }
}

Travel *Moyend(Travel *T, Dist *d, float *x, int *i) {
  T = Moy(T, d, x, i);
  T->moy = *x / *i;
  return T;
}

TownS *maxminmoy(TownS *T) {

  if (T->left != NULL) {
    T->left = maxminmoy(T->left);
  }
  if (T->t != NULL) {
    float x=0;
    int i=0;
    T->t = Max(T->t, T->t->parc);
    T->t = Min(T->t, T->t->parc);
    T->t = Moyend(T->t, T->t->parc, &x, &i);
  }
  if (T->right != NULL) {
    T->right = maxminmoy(T->right);
  }
  return T;
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

int Finalcompare(TownS *A, Travel *T) {
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
  if (f->t == NULL) // quand la chaine est vide
  {
    printf("No town!\n");
    return (NULL);
  } else {
    if (f->t != NULL && f->right != NULL) {
      f->right =supTown2(f->right, passage); // si il existe fils droit alors on cherche à suprrimer le fils droit
      return (f);
    }

    if (f->left != NULL) {
      f->left = supTown2(f->left,passage); // s'il n'existe pas de fils droit mais qu'il existe un fils gauche alors on voir pour supprimer le fils gauche
      return (f);
    }

    else {
      *passage = *f->t; // si aucun fils n'existe: suppression du premier chainon (pour l'envoyer dans l'AVL)                  
      f->t = (Travel *){0};
      free(f);
      return NULL;
    }
  }
}

void parcoursInf(AVLtempo *A, int *x, TownS **C) {
  if ((A->right != NULL && *x < 50) && A != NULL) {
    parcoursInf(A->right, x, C);
  }
  if (*x < 50) {
    *C = avlInsert(*C, A->t, &TownScreate, &Finalcompare, NULL, NULL);
    (*x) += 1;
  }
  if (A != NULL && (A->left != NULL && *x < 50)) {
    parcoursInf(A->left, x, C);
  }
}

void parcourspre(TownS *C) {
  if (C->left != NULL)
    parcourspre(C->left);
  printf("%d;%f;%f;%f\n", C->t->ID, C->t->max, C->t->min, C->t->moy);
  if (C->right != NULL)
    parcourspre(C->right);
}

void computationS(RouteStream *stream) {
  TownS *T=NULL;
  Travel *t;
  AVLtempo *A=NULL;
  int x = 0;
  RouteStep step;
  while (rsRead(stream, &step, ALL_FIELDS & ~TOWN_A & ~TOWN_B & ~STEP_ID)) {
    t = malloc(sizeof(Travel));
    t->ID = step.routeId;
    t->parc = malloc(sizeof(Dist));
    t->parc->d = step.distance;
    T = avlInsert(T, t, TownScreate, Townscompare, NULL, NULL);
    x += 1;
  }
  x = 0;
  T = maxminmoy(T);
  while (T != NULL) {
    t=malloc(sizeof(Travel));
    T = supTown2(T, t); // envoyer les chainons de l'avl primaire dans l'avl principal
    A = avlInsert(A, t, &AVLcreate, &AVLcompare, NULL, NULL);
  }
  T = NULL;
  parcoursInf(A, &x, &T);
  parcourspre(T);
}