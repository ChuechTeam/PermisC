# ARG1 = Input file
# ARG2 = Output file

set style fill solid 0.5
set boxwidth 0.8
set ytics nomirror
set xtics nomirror
set xtics font ",8"
set xtics rotate by -315 right
set ylabel "Distance (km)"
set xlabel "Identifiant du trajet"
set title "Option -s: Distance = f(Route) "
set terminal pngcairo size 800,600
set output ARG2
datafile = ARG1
set yrange [0:*]
set datafile separator ";"

# Plot the data
plot \
    ARG1 using 1:3:5:xticlabels(2) lc rgb "0xFF000000" notitle, \
    ARG1 using 1:5 with filledcurves x1 lc rgb "#99ccff" title "Distances Max/Min (km)", \
    ARG1 using 1:3 with filledcurves x1 lc rgb "white" notitle, \
    ARG1 using 1:4 smooth csplines title "Distance moyenne (km)"


