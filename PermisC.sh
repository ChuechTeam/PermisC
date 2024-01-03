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


# ----------------------------------------------
# Phase 1: Argument checking, parsing and help
# ----------------------------------------------

# First see if we have any help argument. If so, print help and exit.
for arg in "$@"; do
  if [ "${arg}" = "-h" ] || [ "${arg}" = "--help" ]; then
    echo "PermisC, le programme de traitement approuvé par Marcel"
    echo "Utilisation : ./PermisC.sh FICHIER <-d1|-d2|-l|-t|-s>..."
    echo "                                   [--experimental-compute]"
    echo "              avec FICHIER un fichier CSV valide."
    echo "Lit le fichier CSV des trajets et effectue tous les traitements demandés."
    echo "Les graphiques seront créés dans le dossier « images »."
    echo ""
    echo "Options :"
    echo "  -h, --help     Afficher l'aide"
    echo "  -d1            Lancer le traitement D1 : les conducteurs avec le plus de trajets"
    echo "  -d2            Lancer le traitement D2 : les conducteurs avec la plus grande distance parcourue"
    echo "  -l             Lancer le traitement L : les trajets les plus longs"
    echo "  -t             Lancer le traitement T : les villes les plus traversées"
    echo "  -s             Lancer le traitement S : les statistiques sur la distance des trajets"
    echo "  --experimental-compute   Utiliser des implémentations expérimentales de calcul (au lieu de awk)"
    echo "                           plus rapides pour certains traitements: seulement D1 pour le moment."
    echo "Variables d'environnement:"
    echo "  AWK : Le chemin vers l'exécutable awk. Par défaut, « awk »."
    echo "  CLEAN : Force la recompilation de l'exécutable PermisC si sa valeur est 1."
    exit 0
  fi
done

# Check if the file argument is missing: either we have no args, or the first is an option.
# Also check if we don't have a computation argument.
if [ $# -eq 0 ]; then
  echo "Le fichier à traiter est requis. (Utilisez l'argument « -h » pour avoir de l'aide)" >&2
  exit 1
elif [ $# -eq 1 ]; then
  if [[ $1 = "-"*  ]]; then
    echo "Le fichier à traiter est requis et doit être le premier argument."\
         "(Utilisez l'argument « -h » pour avoir de l'aide)" >&2
  else
    echo "Un argument de traitement est requis. (Utilisez l'argument « -h » pour avoir de l'aide)" >&2
  fi
  exit 1
fi

# Check if the CSV file exists, and store it.
CSV_FILE=$1
if [ ! -f "$CSV_FILE" ]; then
  # If it starts with a dash, there's a 99.99% chance that the user put an option instead of a file.
  if [[ "$CSV_FILE" = "-"* ]]; then
    echo "Le fichier à traiter doit être le premier argument. (Utilisez l'argument « -h » pour avoir de l'aide)" >&2
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
EXPERIMENTAL_COMPUTE=0

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

# Put all computations in the COMPUTATIONS array, and parse other arguments as well.
for (( i=2; i<=$#; i++ )); do
  arg=${!i}
  if [ "$arg" = "-d1" ]; then
    add_computation "d1"
  elif [ "$arg" = "-d2" ]; then
    add_computation "d2"
  elif [ "$arg" = "-l" ]; then
    add_computation "l"
  elif [ "$arg" = "-t" ]; then
    add_computation "t"
  elif [ "$arg" = "-s" ]; then
    add_computation "s"
  elif [ "$arg" = "--experimental-compute" ]; then
    EXPERIMENTAL_COMPUTE=1
  elif [[ "$arg" = "-"* ]]; then
    echo "Option « $arg » inconnue. (Utilisez l'argument « -h » pour avoir de l'aide)" >&2
    exit 1
  else
    echo "Un seul fichier peut être donné à la fois. (Utilisez l'argument « -h » pour avoir de l'aide)" >&2
    exit 1
  fi
done


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
  rm -r "$TEMP_DIR"
fi
mkdir -p "$TEMP_DIR"
mkdir -p "$IMAGES_DIR"

# Compile the PermisC executable if it doesn't exist. Output the build to the temp folder.
CLEAN=${CLEAN:-0} # Set a default for the CLEAN variable
export CLEAN # Propagate the CLEAN variable to the make command.
if [ ! -f "$PERMISC_EXEC" ] || [ "$CLEAN" -eq 1 ]; then
  echo -n "Compilation de l'exécutable PermisC..."
  if ! make -C "$PROGC_DIR" --no-print-directory build OPTIMIZE=1 > "$TEMP_DIR/build.log"; then
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

# Allow the user to use a custom awk executable (like mawk), and throw if it doesn't exist.
# TODO: Only check when we're using an awk-based computation?
AWK=${AWK:-awk}
if ! type "$AWK" > /dev/null 2>&1; then
  echo "Impossible de lancer le traitement : commande awk (« $AWK ») introuvable." >&2
  exit 3
fi

# Computations D1, D2 and L are done with awk. T and S are done with the PermisC executable.
# Computation D1 can be done with the experimental implementation in PermisC.
# The 141 exit code should be ignored later as it means the sort command was interrupted by a SIGPIPE,
# which we arguably don't care. (Sorry to break sort's feelings...)

comp_d1() {
  if [ "$EXPERIMENTAL_COMPUTE" -eq 1 ]; then
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
  COMP_NAME=${comp^^} # Uppercase the computation name
  echo -n "Traitement $COMP_NAME en cours..."

  # Seconds and nanoseconds concatenated make a fixed-point decimal number
  TIME_START="$(date +%s%N)"
  if ! comp_dispatch "$comp"; then
    # TEMPORARY: Doesn't work on mac os
    # ERR_FILE="$(realpath --relative-to="$(pwd)" "$(comp_err_file "$comp")")"
    ERR_FILE="$(realpath "$(comp_err_file "$comp")")"
    echo " Échec !"
    echo "Erreur lors du traitement $COMP_NAME. Lisez le fichier $ERR_FILE pour plus de détails." >&2
    exit 3
  fi
  TIME_END="$(date +%s%N)"
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
    COMP_NAME=${comp^^}
    # TEMPORARY: Doesn't work on mac os
    # ERR_FILE="$(realpath --relative-to="$(pwd)" "$(graph_err_file "$comp")")"
    ERR_FILE="$(realpath "$(graph_err_file "$comp")")"
    echo "Erreur lors de la génération du graphique du traitement $COMP_NAME. Lisez le fichier $ERR_FILE pour plus de détails." >&2
    exit 4
  fi
done

echo "Programme terminé ! Les graphiques sont disponibles dans le dossier « images »."