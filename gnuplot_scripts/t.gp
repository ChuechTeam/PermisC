# ARG1 = Input file
# ARG2 = Output file

# Graph style
set style fill solid 0.5
set boxwidth 0.8
set ytics nomirror 
set xtics nomirror
set xtics rotate by -315 right
set ylabel "Nombre de trajets"
set xlabel "Villes"
set title "Option -t : Nb routes = f (Villes)"
set terminal pngcairo size 1314,1320
set output ARG2
myBoxWidth = 0.4

# Load from the input file
datafile = ARG1
set yrange [0:*]

# First column using $1
# Use the ';' as the file separator
set datafile separator ";"
plot datafile using ($2):xtic(sprintf("%s", strcol(1))) with boxes lc rgb 'green' title 'Total routes', \
     '' using ($0+0.1):($3) with boxes lc rgb 'blue' title 'First town'
