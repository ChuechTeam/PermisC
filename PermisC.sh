#!/usr/bin/env bash

# ----------------------------------------------
# Phase 0: Configure the script
# ----------------------------------------------

# Use some sensible defaults for error handling
# that doesn't make me turn around in circles for 1 hour long...

# Failures in individual pipe commands propagate to the entire pipe command.
# The default is DUMB and hides any error in the [0; n-1] commands of the n commands pipe.
set -o pipefail

# Exit the script if any command fails.
set -e

# Exit the script if any variable is undefined.
set -u

if [ ! -v BASH_VERSINFO ]; then
  echo "Ce script doit être lancé avec bash." >&2
  exit 1
fi

# Put the script in "compatibility mode" when we're running on an old version of bash,
# or some UNIX OS other than GNU/Linux.
# Some commands don't work on Mac OS for example, and some bash features are missing,
# so we need to adjust stuff for it to just work.
if [[ "$OSTYPE" != "linux-gnu"* ]] || (( BASH_VERSINFO[0] < 4 )) || [ "${COMPAT_MODE:-0}" -eq 1 ]; then
  echo "Attention : mode de compatibilité activé ! (Bash < 4 ou OS autre que GNU/Linux)" >&2
  COMPAT_MODE=1
else
  COMPAT_MODE=0
fi

# ----------------------------------------------
# Phase 1: Argument checking, parsing and help
# ----------------------------------------------

# Handy function to print an error message for argument errors caused by the user.
print_arg_error() {
  echo "$1 (Utilisez l'argument « -h » pour avoir de l'aide)" >&2
}

# First see if we have any help argument. If so, print help and exit.
for arg in "$@"; do
  if [ "${arg}" = "-h" ] || [ "${arg}" = "--help" ]; then
    ORANGE="\033[38;5;208m" # Orange color
    UL_ON="\033[4m" # Underline on
    UL_OFF="\033[24m" # Underline off
    RESET="\033[0m" # Reset everything
    echo -e \
"PermisC, le programme de traitement qui va à toute vitesse (approuvé par Marcel !)
Utilisation : ./PermisC.sh FICHIER <-d1|-d2|-l|-t|-s>...
                                   [options]
              avec FICHIER un fichier CSV valide.
Lit le fichier CSV des trajets et effectue tous les traitements demandés.
Les graphiques seront créés dans le dossier « images ».
Options :
  -h, --help                 Afficher l'aide
  -d1                        Lancer le traitement D1 : les conducteurs avec le plus de trajets
  -d2                        Lancer le traitement D2 : les conducteurs avec la plus grande distance parcourue
  -l                         Lancer le traitement L : les trajets les plus longs
  -t                         Lancer le traitement T : les villes les plus traversées
  -s                         Lancer le traitement S : les statistiques sur la distance des trajets
  -Q, --quick [n]            Utiliser des implémentations natives de calcul (au lieu de awk) plus rapides
                             pour le traitement D1, de plus en plus avancées selon le niveau choisi :
                                 0 : Utiliser awk si possible
                                 1 : Utiliser les implémentations basiques en C (AVL uniquement)
                                 2 : Utiliser les implémentations avancées en C (table de hachage, expérimental !)
                                 3 : Utiliser les implémentations très avancées en C (SSE/AVX, ultra expérimental !)
                             Un changement de niveau peut nécessiter une recompilation du programme.
  -X, --experimental         Équivalent à --quick 2.
  -E, --exceed-speed-limits, Équivalent à --quick 3.
      --excès-de-vitesse     ${ORANGE}${UL_ON}Attention${UL_OFF} : Cette option ajoute des propulseurs surpuissants à votre camion
                                         et vous expose à une amende pour excès de vitesse sur le
                                         périphérique nord de Rennes !!
$RESET
Variables d'environnement :
  AWK : Le chemin vers l'exécutable awk. L'exécutable mawk est utilisé si possible."
    exit 0
  fi
done

# Check if the file argument is missing: either we have no args, or the first is an option.
# Also check if we don't have a computation argument.
if [ $# -eq 0 ]; then
  print_arg_error "Le fichier à traiter est requis."
  exit 1
elif [ $# -eq 1 ]; then
  if [[ $1 = "-"*  ]]; then
    print_arg_error "Le fichier à traiter est requis et doit être le premier argument."
  else
    print_arg_error "Un argument de traitement est requis."
  fi
  exit 1
fi

# Check if the CSV file exists, and store it.
CSV_FILE=$1
if [ ! -f "$CSV_FILE" ]; then
  # If it starts with a dash, there's a 99.99% chance that the user put an option instead of a file.
  if [[ "$CSV_FILE" = "-"* ]]; then
    print_arg_error "Le fichier à traiter doit être le premier argument."
  else
    echo "Le fichier « $CSV_FILE » n'existe pas." >&2
  fi
  exit 1
else
  # Put the absolute path, so we don't get sneaky errors with argument-parsing for awk and PermisC,
  # or when the current directory changes for some reason.
  CSV_FILE="$(realpath "$CSV_FILE")"
fi

COMPUTATIONS=()
QUICK_LEVEL=0

# Adds a computation to the COMPUTATIONS array, while ignoring duplicates.
add_computation() {
  local comp="$1"
  for c in "${COMPUTATIONS[@]}"; do
    if [ "$c" = "$comp" ]; then
      return
    fi
  done

  COMPUTATIONS+=("$comp")
}

# Checks if the string is an unsigned integer.
# Taken from: https://stackoverflow.com/a/61835747/5816295
is_number() {
  case $1 in ''|*[!0-9]*) return 1;;esac;
}

# Put all computations in the COMPUTATIONS array, and parse other arguments as well.
for (( i=2; i<=$#; i++ )); do
  arg=${!i}
  case "$arg" in
  -d1|-d2|-l|-t|-s)
      add_computation "${arg:1}" ;;
  --quick|-Q*)
      # Parse the quickness level.
      NEXT=$(( i+1 )); SKIP_NEXT=0; SHORT_ARG=0
      if [[ "$arg" = "-Q"* ]]; then
        # Try using the number in the argument (like in -Q2 for example)
        QL_STR="${arg:2}"
        SHORT_ARG=1
      fi
      if ! is_number "${QL_STR:-}" && (( i != $# )); then
        if [ $SHORT_ARG -eq 1 ] && [ -n "$QL_STR" ]; then
          print_arg_error "Le nombre suité après « -Q » (« $QL_STR ») est invalide."
          exit 1
        fi
        QL_STR="${!NEXT}"
        SKIP_NEXT=1
      fi
      if is_number "${QL_STR:-}"; then
        QUICK_LEVEL="$QL_STR"
        if [ $SKIP_NEXT -eq 1 ]; then
          i=NEXT
        fi
      else
        QUICK_LEVEL=1
      fi ;;
  --experimental|-X)
      QUICK_LEVEL=2 ;;
  --exceed-speed-limits|--excès-de-vitesse|-E) # Little easter egg (not anymore...)
      QUICK_LEVEL=3 ;;
  -*)
      print_arg_error "Option « $arg » inconnue."
      exit 1 ;;
  *)
      print_arg_error "Un seul fichier peut être donné à la fois."
      exit 1 ;;
  esac
done

if [ "${#COMPUTATIONS[@]}" -eq 0 ]; then
  print_arg_error "Un argument de traitement est requis."
  exit 1
fi

# Prepare some handy variables for handling quickness levels.
# QLx is equal to 1 if QUICK_LEVEL >= x, and 0 otherwise.
# Use the "10#" prefix to force the number to be interpreted as a decimal, in case
# the user puts a zero in front of the number (I actually did by the way).
QL1=$(( 10#$QUICK_LEVEL >= 1 ))
QL2=$(( 10#$QUICK_LEVEL >= 2 ))
QL3=$(( 10#$QUICK_LEVEL >= 3 ))

# ----------------------------------------------
# Phase 2: Make compilation and folder setup
# ----------------------------------------------

# Setup some variables for directories
# Use realpath to find the absolute path to avoid surprises once again.
# The -q argument mutes the stderr output, and yes it might fail due to an invalid
# $0 argument or a too long path.
if ! PROJECT_DIR="$(realpath -q "$(dirname "$0")")"; then
  echo "Impossible de trouver le dossier racine du projet" >&2
  exit 2
fi
PROGC_DIR="$PROJECT_DIR/progc"
TEMP_DIR="$PROJECT_DIR/temp"
IMAGES_DIR="$PROJECT_DIR/images"
AWK_COMP_DIR="$PROJECT_DIR/awk_computations"
GNUPLOT_SCRIPTS_DIR="$PROJECT_DIR/gnuplot_scripts"
PERMISC_EXEC="$PROGC_DIR/build-make/PermisC"

# Putting this JUST IN CASE $0 doesn't give the right directory due to obscure
# reasons (aliased script, not using bash, symlinks, etc.)
# If we didn't check this, who knows what would be DELETED with rm?!?
if [ ! -f "$PROJECT_DIR/PermisC.sh" ]; then
  echo "Impossible de trouver le dossier racine du projet" >&2
  exit 2
fi

# Setup the default folders: temp and images. Clear the temp folder beforehand.
if [ -d "$TEMP_DIR" ]; then
  rm -rf "$TEMP_DIR" # Ignore failures if we can't delete the folder or some annoying file.
fi
mkdir -p "$TEMP_DIR"
mkdir -p "$IMAGES_DIR"

# Setup variables for the make build, depending on the quickness level.

# Force an optimized build. We don't need debugging for this script! (But leave it configurable)
export OPTIMIZE=${OPTIMIZE:-1}
# Use native optimizations by default (-march=native), we aren't going to distribute the executable anyway.
export OPTIMIZE_NATIVE=${OPTIMIZE_NATIVE:-1}
# Allow the user to configure the CLEAN variable, defaulting to 0.
export CLEAN=${CLEAN:-0}
# Enable experimental algorithms for QUICK_LEVEL >= 2.
export EXPERIMENTAL_ALGO=${EXPERIMENTAL_ALGO:-$QL2}
# Enable experimental AVX stuff for QUICK_LEVEL >= 3.
export EXPERIMENTAL_ALGO_AVX=${EXPERIMENTAL_ALGO_AVX:-$QL3}

# Compile the PermisC executable if one of these conditions is true:
# - There's no executable
# - The build variables have changed (if the quickness level changed, we need to recompile)
# - The user wants to force a recompilation (using the CLEAN variable)
if [ ! -f "$PERMISC_EXEC" ] || ! make -C "$PROGC_DIR" check_vars > /dev/null 2>&1 || [ "$CLEAN" -eq 1 ]; then
  echo -n "Compilation de l'exécutable PermisC..."
  if ! make -C "$PROGC_DIR" --no-print-directory build > "$TEMP_DIR/build.log" 2>&1; then
    echo " Échec !"
    echo "Erreur lors de la compilation de PermisC. Lisez le fichier temp/build.log pour plus de détails." >&2
    exit 2
  else
    echo " Terminé !"
  fi
fi

# ----------------------------------------------
# Phase 3: Run the computations
# ----------------------------------------------

# Returns the output file, in the temp folder, for a computation.
comp_out_file() {
  local comp="$1"
  echo "$TEMP_DIR/result_$comp.out"
}

# Returns the error file (the file where the stderr is redirected) for a computation.
comp_err_file() {
  local comp="$1"
  echo "$TEMP_DIR/result_$comp.err"
}

# Basically tries to put the computation name in uppercase.
comp_name() {
  if [ $COMPAT_MODE -eq 0 ]; then
    local val="$1"
    echo "${val^^}"
  else
    # Try to use tr for this instead.
    if ! echo "$1" | tr '[:lower:]' '[:upper:]' 2> /dev/null; then
      # Oh my god really? I give up.
      echo "$1"
    fi
  fi
}

# Returns the current time in seconds+nanoseconds as a fixed-point decimal.
# Precision can be lower on not-really-supported systems.
measure_time() {
  if [ $COMPAT_MODE -eq 0 ]; then
    date +%s%N
  else
    # Nanoseconds are not available on Mac OS, so we try to use perl, or just seconds if it fails.
    # Taken from: https://stackoverflow.com/a/15328160/5816295 (i hope it works?) (i adjusted it)
    local nanos
    if ! nanos=$(perl -MTime::HiRes -e 'printf("%.0f\n",Time::HiRes::time()*1000000000)' 2> /dev/null); then
      nanos=$(( $(date +%s) * 1000000000 ))
    fi;
    echo "$nanos"
  fi
}

# Prints the path to the file, relative to the current directory.
simple_path() {
  if [ $COMPAT_MODE -eq 0 ]; then
    realpath --relative-to="$(pwd)" "$1"
  else
    echo "$1"
  fi
}

# Allow the user to use a custom awk executable, and use mawk by default, if possible.
if [ ! -v AWK ]; then
  if type mawk > /dev/null 2>&1; then
    AWK="mawk"
  else
    AWK="awk"
  fi
fi
if ! type "$AWK" > /dev/null 2>&1; then
  echo "Impossible de lancer le traitement : commande awk (« $AWK ») introuvable." >&2
  exit 3
fi

# Computations D1, D2 and L are done with awk. T and S are done with the PermisC executable.
# Computation D1 can be done with the experimental implementation in PermisC.
# The 141 exit code should be ignored later as it means the sort command was interrupted by a SIGPIPE,
# which we arguably don't care. (Sorry to break sort's feelings...)

comp_d1() {
  if [ $QL1 -eq 1 ]; then
    "$PERMISC_EXEC" -d1 "$CSV_FILE"
  else
    $AWK -F ';' -f "$AWK_COMP_DIR/d1.awk" "$CSV_FILE" | LC_ALL=C sort -t ';' -k2nr -S 50% | head -n 10
  fi
  return $?
}

comp_d2() {
  $AWK -F ';' -f "$AWK_COMP_DIR/d2.awk" "$CSV_FILE" | sort -t ';' -k2 -nr | head -n 10
  return $?
}

comp_l() {
  $AWK -F ';' -f "$AWK_COMP_DIR/l.awk" "$CSV_FILE" | sort -t ';' -k2nr | head -n 10 | sort -t ';' -k1,1n
  return $?
}

# Calls the adequate function for a computation. Also this is a separate function
# so errors are handled in an easier way.
comp_dispatch() {
  local comp="$1"
  local -r out_file="$(comp_out_file "$comp")"
  local -r err_file="$(comp_err_file "$comp")"
  case "$comp" in
    d1)
      comp_d1 > "$out_file" 2> "$err_file"
      ;;
    d2)
      comp_d2 > "$out_file" 2> "$err_file"
      ;;
    l)
      comp_l > "$out_file" 2> "$err_file"
      ;;
    t)
      "$PERMISC_EXEC" -t "$CSV_FILE" > "$out_file" 2> "$err_file"
      ;;
    s)
      "$PERMISC_EXEC" -s "$CSV_FILE" > "$out_file" 2> "$err_file"
      ;;
  esac
  RET=$?
  # Mark SIGPIPE return values as a success, it just means that the sort command was interrupted by head.
  if [ $RET -eq 141 ]; then
    RET=0
  fi;

  if [ $RET -ne 0 ]; then
    echo "[ Fin | Code d'erreur : $RET ]" >> "$err_file"
  else
    # Remove the empty error file if everything went fine.
    rm -f "$err_file"
  fi
  return $RET
}

# Iterate through all the computations, and run them one by one.
# Also measure the time it takes to run each computation.
for comp in "${COMPUTATIONS[@]}"; do
  COMP_NAME=$(comp_name "$comp") # Uppercase the computation name
  echo -n "Traitement $COMP_NAME en cours..."

  # Seconds and nanoseconds concatenated make a fixed-point decimal number
  TIME_START="$(measure_time)"
  if ! comp_dispatch "$comp"; then
    ERR_FILE="$(simple_path "$(comp_err_file "$comp")")"
    TIME_END="$(measure_time)"
    ELAPSED_MS=$(( (TIME_END - TIME_START)/1000000 ))
    echo " Échec ! (en $ELAPSED_MS ms)"
    echo "Erreur lors du traitement $COMP_NAME. Lisez le fichier $ERR_FILE pour plus de détails." >&2
    exit 3
  fi
  TIME_END="$(measure_time)"
  ELAPSED_MS=$(( (TIME_END - TIME_START)/1000000 ))
  echo " Terminé en $ELAPSED_MS ms !"
done


# ----------------------------------------------
# Phase 4: Draw the graphs using gnuplot
# ----------------------------------------------

graph_out_file() {
  local comp="$1"
  echo "$IMAGES_DIR/graph_$comp.png"
}

graph_err_file() {
  local comp="$1"
  echo "$TEMP_DIR/graph_$comp.err" # Look closely, it's temp, not images!
}

graph_dispatch() {
  local ret
  local -r comp="$1"
  local -r in_file="$(comp_out_file "$comp")"
  local -r out_file="$(graph_out_file "$comp")"
  local -r err_file="$(graph_err_file "$comp")"
  gnuplot -c "$GNUPLOT_SCRIPTS_DIR/$comp.gp" "$in_file" "$out_file" 2> "$err_file"
  ret=$?
  if [ $ret -ne 0 ]; then
    echo "[ Fin | Code d'erreur : $ret ]" >> "$err_file"
  else
    rm -f "$err_file"
  fi
  return $ret
}

# Draw the graph for each computation
echo "Génération des graphiques..."
for comp in "${COMPUTATIONS[@]}"; do
  if ! graph_dispatch "$comp"; then
    COMP_NAME=$(comp_name "$comp")
    ERR_FILE="$(simple_path "$(graph_err_file "$comp")")"
    echo "Erreur lors de la génération du graphique du traitement $COMP_NAME. Lisez le fichier $ERR_FILE pour plus de détails." >&2
    exit 4
  fi
done

echo "Programme terminé ! Les graphiques sont disponibles dans le dossier « images »."