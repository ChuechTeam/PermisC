# ARG1 = Input file
# ARG2 = Output file

# Graph style
set style fill solid 0.5
set boxwidth 0.8
set ytics nomirror 
set xtics nomirror
set xtics font ",12"
set xtics rotate by -315 right
set ylabel "Nombre de trajets"
set ylabel font ",14"
set xlabel font ",14"
set title font ",15"
set xlabel "Villes"
set title "Option -t : Nb routes = f (Villes)"
set terminal pngcairo size 1310,1000
set output ARG2
# Use the ';' as the file separator
set datafile separator ";"

# Load from the input file
datafile = ARG1
set yrange [0:*]

# First column using $1

plot datafile using ($2):xtic(sprintf("%s", strcol(1))) with boxes lc rgb 'green' title 'Trajets totaux', \
     '' using ($0+0.1):($3) with boxes lc rgb 'blue' title 'Première ville'
