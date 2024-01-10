# Présentation du projet

## Répartition des tâches

- Paul P. : scripts Gnuplot pour tous les traitements, traitements D1, D2 et L sous Awk
- Yani A. : lecture du fichier CSV, implémentation AVL, Makefile, script Bash, bonus
- Mathis M. : traitements T et S en C

## Planning de réalisation

### Jalon 1 : Préparation du terrain
- **Programme C**
  - [x] Programme en C basique [YA]
    - [x] Makefile simple
    - [x] Lecture du fichier CSV
    - [x] Implémentation complète et optimisée des arbres AVL (hors suppression)
    - [x] Lecture des arguments et lancement du traitement (avec une implémentation vide)
- **Scripts (Shell, Awk, Gnuplot)**
  - [x] Traitements avec Awk [PP]
    - [x] Traitement D1
    - [x] Traitement D2
    - [x] Traitement L

### Jalon 2 : Les premiers traitements fonctionnels, de la commande jusqu'au graphique
- **Programme C**
  - [x] Traitement D1 en C, avec AVL (bonus, à but d'exemple) [YA]
  - [x] Traitement T en C [MM]
- **Scripts (Shell, Awk, Gnuplot)**
  - [x] Script Bash pour compiler, lancer les traitements, et générer les graphiques [YA]
  - [x] Scripts Gnuplot pour les premiers traitements [PP]
    - [x] Traitement D1
    - [x] Traitement D2
    - [x] Traitement L
    - [x] Traitement T

### Jalon 3 : Un programme complet !
- **Programme C**
    - [x] Traitement S en C [MM]
    - [x] Optimisation de la lecture CSV et meilleure gestion des erreurs (bonus) [YA]
    - [x] Traitement D1 en C, avec table de hachage, expérimental (bonus) [YA]
- **Scripts (Shell, Awk, Gnuplot)**
    - [x] Script Gnuplot pour le traitement S [PP]
    - [x] Compatibilité macOS du script Bash (bonus) [PP & YA]
    - [x] Ajout de l'option `-Q`/`--quick` pour choisir les algorithmes utilisés (bonus) [YA]

### Jalon 4 : Polissage final

- **Programme C**
    - [ ] Vérification entière du code, renforcement de la robustesse et de la gestion d'erreur [YA]
    - [ ] Algorithme expérimental pour le traitement T, plus rapide, avec des AVL (bonus) [YA]
- **Scripts (Shell, Awk, Gnuplot)**
    - [ ] Embellissement de l'expérience utilisateur du script Bash (bonus) [?]
      - [ ] Messages colorés, avec des emojis
      - [ ] Liens cliquables vers les graphiques et autres fichiers
      - [ ] Vérifications supplémentaires sur les dépendances requises (gnuplot, make)

### Jalon 5 : Toujours plus vite ?! (Bonus, si on a le temps)
- **Programme C**
- [ ] Traitements D2 et L en C, avec des AVL et des tables de hachage (pour `-Q1`)  [YA]

### Jalon 6 : BEAUCOUP TROP RAPIDE !! (Bonus, si on a beaucoup de temps)
- **Programme C**
- [ ] Implémentation multi-cœurs des traitements, avec pthreads [YA]