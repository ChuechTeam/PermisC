# ARG1 = Input file
# ARG2 = Output file

# Graph style settings
set style fill solid 0.5
set boxwidth 0.8
set ytics nomirror 
set xtics nomirror
set ylabel "Conducteurs"
set xlabel "Nombre de trajets"
set title "Option -d1 : Nombre de trajets par conducteur (Top 10)"
set terminal pngcairo size 800,600
set output ARG2 # Out
set datafile separator ";"

# Horizontal bar graph technique taken from
# https://stackoverflow.com/questions/62848395/horizontal-bar-chart-in-gnuplot/62854722#62854722 
set yrange [0:*] reverse      # start at zero, find max from the data (reversed)
set style fill solid          # solid color boxes
unset key                     # turn off all titles

myBoxWidth = 0.8
set offsets 0,0,0.5-myBoxWidth/2.,0.5

# x y dx dy
plot ARG1 using\
    (0.5*$2):\
    0:\
    (0.5*$2):\
    (myBoxWidth/2.):\
    ytic(strcol(1))\
with boxxy lc rgb 'green'
