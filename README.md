# Permis C
Un programme de traitement de trajets de camion très très rapide !

## Plateformes supportées
- Linux
- macOS (support approximatif)

Windows n'est pas supporté, il est donc recommandé d'installer WSL pour utiliser Permis C. 
Néanmoins, il existe un fichier CMake pour pouvoir compiler le programme C sur Windows et l'utiliser, mais la génération
de graphiques est impossible.

## Prérequis
- Un compilateur C (gcc, clang, ...), inclus dans le paquet `build-essential` sur Debian compatible avec le standard C11
- Make, inclus dans le paquet `build-essential` sur Debian
- Gnuplot 5.0 ou plus récent, paquet `gnuplot` sur Debian
- Une implémentation d'awk, mawk est vivement recommandé et choisi si possible
- Bash 4.0+ (3.0+ est possible mais certaines fonctionnalités seront manquantes)

## Téléchargement
Pour télécharger Permis C, clonez le dépôt git :
```bash
git clone https://github.com/ChuechTeam/PermisC.git
```

Les fichiers de Permis C seront disponibles dans le dossier `PermisC`. 
Pour la suite de ce README, on supposera que ce dossier est le dossier courant.

## Utilisation

Lancez le script `PermisC.sh` avec le fichier CSV en premier argument, 
puis les traitements voulus en arguments (`-d1`, `-d2`, `-l`, `-t`, `-s`). Par exemple,

```bash
# Lance les traitements D1, L et T avec le fichier data.csv.
./PermisC.sh data.csv -d1 -l -t
```

Une fois les traitements terminés et les graphiques générés, les graphiques seront disponibles dans le dossier `images`, 
à l'intérieur du dossier de Permis C. Les résultats des traitements sont disponibles dans le dossier `temp`.

En supplément, l'argument `-Q` ou `--quick` permet d'utiliser des algorithmes plus rapides ! Il est possible de spécifier
un nombre de 0 à 3 après l'argument `-Q`, ce qui permet d'utiliser des algorithmes de plus en plus expérimentaux, 
mais aussi de plus en plus efficaces :

- `-Q0` : utilise les algorithmes par défaut, utilisant Awk si possible, ou l'algorithme de base en C sinon :
  - Awk : pour D1, D2 et L
  - Algorithme C de base : T et S
- `-Q1` : utilise les algorithmes expérimentaux en C, pouvant utiliser des [tables de hachage](https://fr.wikipedia.org/wiki/Table_de_hachage) :
  - Algorithme C expérimental : tous les traitements ! (D1, D2, L, T et S)
- `-Q2` : active les [instructions AVX2](https://fr.wikipedia.org/wiki/Advanced_Vector_Extensions)
pour une lecture de fichier plus rapide ; requiert un processeur compatible AVX2

En dehors du README, l'aide reste disponible en lançant le script avec l'argument `-h` ou `--help`.

## Compilation

Le script `PermisC.sh` s'occupe automatiquement de la compilation. Il est
toujours possible de compiler manuellement le programme pour plus de contrôle sur les options du Makefile en suivant
ces instructions :

1. Se placer dans le dossier `progc/`
2. Lancer la commande `make -j build`

Différentes variables sont configurables pour personnaliser la compilation :
- `CC` : le compilateur C utilisé (`gcc` par défaut)
- `CFLAGS` : les options envoyées au compilateur
- `OPTIMIZE` : activer les optimisations du compilateur si mis à 1 (0 par défaut)
- `OPTIMIZE_NATIVE` : activer les optimisations spécifiques au processeur de l'ordinateur si mis à 1 (0 par défaut)
- `EXPERIMENTAL_ALGO` : active les algorithmes marqués comme « expérimentaux » si mis à 1 (0 par défaut)
- `EXPERIMENTAL_ALGO_AVX` : active les algorithmes expérimentaux utilsant AVX si mis à 1 
(0 par défaut, nécessite `OPTIMIZE_NATIVE` et un processeur capable d'utiliser AVX)
- `ASM` : génère le code assembleur du programme si mis à 1 (0 par défaut)

Pour configurer ces variables, il suffit de les définir avant de lancer la commande `make`.
Par exemple, `OPTIMIZE=1 ASM=1 make -j build`.

Chaque changement de variable entraîne une recompilation complète du programme.

Il existe aussi des scripts utiles pour lancer le programme C, dans le dossier `progc` : 
- `run.sh` pour compiler et lancer le programme
- `debug.sh` pour compiler et lancer le débogueur `gdb` sur le programme

Tous les arguments passés à ces scripts sont directement passés au programme C. 
Les variables de compilation seront aussi données au Makefile.