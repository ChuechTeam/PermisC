#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct Dist {
  AVL_HEADER(Dist);
  int d;
} Dist;

typedef struct Travel {
  int ID;
  int min;
  int max;
  int moy;
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

Dist *Distcreate(int parc) {
  Dist *A = malloc(sizeof(Dist)); // création du chainon de l'AVL
  assert(A);
  A->d = parc;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int distcompare(Dist *p, int i) {
  if (i > p->d) {
    return -4;
  }
  if (i < p->d) {
    return 1;
  } else {
    return 0;
  }
}

int distsearch(Dist *p, int i) {
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
  TownS *A = malloc(sizeof(Dist)); // création du chainon de l'AVL
  assert(A);
  A->t = T;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int Townscompare(TownS *T, Travel *i) {
  if (i->ID > T->t->ID) {
    return -4;
  }
  if (i->ID < T->t->ID) {
    return 1;
  } else {
    if (distsearch(T->t->parc, i->parc->d) == 0) {
      T->t == avlInsert(T->t->parc, i->parc->d, &Distcreate, distcompare, NULL,
                        NULL);
    }
    return 0;
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

Travel *Moy(Travel *T, Dist *d, int *x, int *i) {
  if (d->right != NULL) {
    T = Moy(T, d->right, x, i);
  }
  if (d->d != NULL) {
    *x += d->d;
    i += 1;
  }
  if (d->left != NULL) {
    T = Moy(T, d->left, x, i);
  }
}

Travel *Moyend(Travel *T, Dist *d, int *x, int *i) {
  T = Moy(T, d, x, i);
  T->moy = *x / *i;
}

AVLtempo *AVLcreate(Travel *T) {
  AVLtempo *A = malloc(sizeof(Dist)); // création du chainon de l'AVL
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

int Finalcompare(TownS* A, Travel* T) {
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

void parcoursInfInv(AVLtempo *A, int *x, TownS** C) {
  if ((A->right != NULL && *x < 50) && A != NULL) {
    parcoursInfInv(A->right, x, C);
  }
  if (*x < 50) {
    *C = avlInsert(*C, &A->t, &TownScreate, &Finalcompare, NULL, NULL);
    (*x) += 1;
  }
  if (A != NULL && (A->left != NULL && *x < 50)) {
    parcoursInfInv(A->left, x, C);
  }
}

void parcoursprefixe(TownS *C) {
  if (C->left != NULL)
    parcoursprefixe(C->left);
  printf("%d;%d;%d;%d\n", C->t->ID, C->t->max, C->t->min, C->t->moy);
  if (C->right != NULL)
    parcoursprefixe(C->right);
}

void computationS(RouteStream *stream) {
  while (rsRead(stream, &step, ALL_FIELDS & ~DISTANCE)) {
    TownS *T = malloc(sizeof(TownS));
  }
}