# ARG1 = Input file
# ARG2 = Output file

# Graph style settings
set style fill solid 0.5
set boxwidth 0.8
set ytics nomirror 
set xtics nomirror
set ylabel "Conducteurs"
set xlabel "Distance parcourue"
set title "Option -d2 : Distance parcourue par conducteur (Top 10)"
set terminal pngcairo size 800,600
set output ARG2 # Out

# Horizontal bar graph technique taken from
# https://stackoverflow.com/questions/62848395/horizontal-bar-chart-in-gnuplot/62854722#62854722 
set yrange [0:*]      # start at zero, find max from the data
set style fill solid  # solid color boxes
unset key             # turn off all titles

myBoxWidth = 0.8
set offsets 0,0,0.5-myBoxWidth/2.,0.5

# x  y dx dy
# !! le ".in"
plot "<awk '{a[i++]=$0} END {for (j=i-1; j>=0;) print a[j--] }' ".ARG1 using\
    (0.5*$3):\
    0:\
    (0.5*$3):\
    (myBoxWidth/2.):\
    ytic(sprintf("%s %s", strcol(1), strcol(2)))\
with boxxy lc rgb 'green'