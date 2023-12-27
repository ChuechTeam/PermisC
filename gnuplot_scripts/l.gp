# ARG1 = Input file
# ARG2 = Output file

# Définir le style du graphique
set style fill solid 0.5
set boxwidth 0.8
set ytics nomirror 
set xtics nomirror
set ylabel "Distance (km)"
set xlabel "Route ID"
set title "Option -l : Distance = f (Route)"
set terminal pngcairo size 800,600
set output ARG2 # Out

set yrange [0:*]      # start at zero, find max from the data
set style fill solid  # solid color boxes
unset key             # turn off all titles

myBoxWidth = 0.8


# x  y dx dy
# !! le ".in"
plot "<awk '{a[i++]=$0} END {for (j=i-1; j>=0;) print a[j--] }' ".ARG1 using\
    0:\
    ($2):\
    (myBoxWidth/2.):\
    xtic(sprintf("%s", strcol(1)))\
with boxes lc rgb 'green'
