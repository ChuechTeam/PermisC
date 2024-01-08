#include "avl.h"
#include "computations.h"
#include "route.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct ID // AVL d'ID
{
  AVL_HEADER(ID)
  int iD;
} ID;

typedef struct Town {
  char name[100];
  struct Town *next;
  int passed; // compteur
  int firstTown;
  ID *IDhead; // chaine d'ID
} Town;

typedef struct AVL1 // AVL primaire
{
  AVL_HEADER(AVL1)
  Town value;
} AVL1;

typedef struct AVLT // AVL principal
{
  AVL_HEADER(AVLT)
  Town value;
} AVLT;

int croiCompare(AVL1 *A,
                Town *i) // comparer les ID entre celui d'un chainon de l'AVL
                         // d'ID et celui d'une ville à ajouter dans l'avl
{
  if (strcmp(i->name, A->value.name) > 0) {
    return -4;
  }
  if (strcmp(i->name, A->value.name) < 0) {
    return 1;
  } else {
    return 0;
  }
}

ID *IDCreate(int i) {
  ID *A = malloc(sizeof(ID)); // création de l'AVL d'ID
  assert(A);
  A->iD = i;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int IDCompare(ID *A, int i) // comparer les ID entre celui d'un chainon de l'AVL
                            // d'ID et celui d'une ville à ajouter dans l'avl
{
  if (i > A->iD) {
    return -4;
  }
  if (i < A->iD) {
    return 1;
  } else {
    return 0;
  }
}

int searchID(ID *h, int Id) {
  ID *p1 = h;
  if (p1 != NULL) {
    if (p1->iD == Id) // Si l'ID existe dans la chaine
    {
      return 1;
    } else {
      if (p1->iD > Id) {
        if (p1->left != NULL) {
          return searchID(p1->left, Id); // recherche de l'ID dans la partie
                                         // droite du sous-arbre si plus grand
        } else {
          return 0;
        }
      }
      if (p1->iD < Id) {
        if (p1->right != NULL) {
          return searchID(p1->right, Id); // sinon la partie gauche si plus
                                          // petit
        } else {
          return 0;
        }
      }
    }
    return 0; // sinon
  }
  return 0;
}

AVL1 *supTown(AVL1 *f, Town *passage) {
  if (f->value.name == NULL) // quand la chaine est vide
  {
    printf("No town!\n");
    return (NULL);
  } else {
    if (f->value.name != NULL && f->right != NULL) {
      f->right =
          supTown(f->right, passage); // si il existe fils droit alors on
                                      // cherche à suprrimer le fils droit
      return (f);
    }

    if (f->left != NULL) {
      f->left = supTown(
          f->left,
          passage); // s'il n'existe pas de fils droit mais qu'il existe un fils
                    // gauche alors on voir pour supprimer le fils gauche
                    //
      return (f);
    }

    else {
      *passage = f->value; // si aucun fils n'existe: suppression du premier
                           // chainon (pour l'envoyer dans l'AVL)
      f->value = (Town){0};
      free(f);
      return NULL;
    }
    return f;
  }
}

AVL1 *AVL1Create(Town *T) {
  AVL1 *A = malloc(sizeof(AVL1)); // création d'un chainon de l'AVL temporaire
  assert(A);
  A->value = *T;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int AVL1Compare(AVLT *A, Town *T) {
  if (strcmp(A->value.name, T->name) <
      0) // si le nom est avant dans l'alphabet alors il ira à gauche
  {
    return -4;
  }
  if (strcmp(A->value.name, T->name) >
      0) // si le nom est après dans l'alphabet alors il ira à droite
  {
    return 1;
  } else {
    if (searchID(A->value.IDhead, T->IDhead->iD) ==
        0) { // si le nom est le meme alors passage +1 si l'ID n'est pas déjà
             // dans l'AVL d'ID
      A->value.passed += 1;
      A->value.IDhead = avlInsert(A->value.IDhead, T->IDhead->iD, &IDCreate,
                                  &IDCompare, NULL, NULL);
    }
    A->value.firstTown += T->firstTown;
    return 0;
  }
}

AVLT *AVLCreate(Town *T) {
  AVLT *A = malloc(sizeof(AVLT)); // création du chainon de l'AVL
  assert(A);
  A->value = *T;
  A->left = NULL;
  A->right = NULL;
  A->balance = 0;
  return A;
}

int AVLCompare(AVLT *A, Town *T) {
  if (T->passed > A->value.passed) // si le chainon est moins parcouru dans ce
                                   // cas ce sera mis sur la gauche
  {
    return -4;
  }
  if (T->passed < A->value.passed) // si le chainon est plus parcouru dans ce
                                   // cas ce sera mis sur la droite
  {
    return 1;
  } else {
    return strcmp(
        A->value.name,
        T->name); // si c'est aussi parcouru ça dépend du nom de la ville
  }
}

void parcoursInfInv(AVLT *A, int *x, AVL1 **C) {
  if ((A->right != NULL && *x < 10) && A != NULL) {
    parcoursInfInv(A->right, x, C);
  }
  if (*x < 10) {
    *C = avlInsert(*C, &A->value, &AVL1Create, &croiCompare, NULL, NULL);
    (*x) += 1;
  }
  if (A != NULL && (A->left != NULL && *x < 10)) {
    parcoursInfInv(A->left, x, C);
  }
}

void parcoursprefixe(AVL1 *C) {
  if (C->left != NULL)
    parcoursprefixe(C->left);
  printf("%s;%d;%d\n", C->value.name, C->value.passed, C->value.firstTown);
  if (C->right != NULL)
    parcoursprefixe(C->right);
}

void computationT(RouteStream *stream) {
  int x = 0;
  Town *T;
  AVL1 *f = malloc(sizeof(AVL1));
  AVLT *A;
  A = NULL;
  f = NULL;
  RouteStep step;
  while (rsRead(stream, &step, ALL_FIELDS & ~DISTANCE)) {
    T = malloc(sizeof(Town));
    strcpy(T->name, step.townA);
    T->IDhead = malloc(sizeof(ID));
    T->IDhead->iD = NULL;
    T->IDhead->left = NULL;
    T->IDhead->right = NULL;
    T->IDhead->balance = NULL;
    T->IDhead->iD = step.routeId; // création d'un chainon ville pour l'envoyer
                                  // dans l'avl primaire
    T->passed = 1;
    if (step.stepId == 1) {
      T->firstTown = 1;
    } else {
      T->firstTown = 0;
    }
    f = f = avlInsert(f, T, &AVL1Create, &AVL1Compare, NULL, NULL);
    T = malloc(sizeof(Town));
    T->IDhead = malloc(sizeof(ID));
    T->IDhead->iD = NULL;
    T->IDhead->left = NULL;
    T->IDhead->right = NULL;
    T->IDhead->balance = NULL;
    strcpy(T->name, step.townB);
    T->IDhead->iD = step.routeId; // création d'un chainon ville pour l'envoyer
                                  // dans l'avl primaire
    T->passed = 1;
    T->firstTown = 0;
    f = avlInsert(f, T, &AVL1Create, &AVL1Compare, NULL, NULL);
  }
  while (f != NULL) {
    f = supTown(
        f, T); // envoyer les chainons de l'avl primaire dans l'avl principal
    A = avlInsert(A, T, &AVLCreate, &AVLCompare, NULL, NULL);
  }
  free(f);
  f = NULL;
  parcoursInfInv(A, &x, &f);
  parcoursprefixe(
      f); // lecture des 10 villes les plus parcourues dans l'ordre alphabétique
}