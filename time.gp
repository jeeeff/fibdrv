reset

set xlabel 'Number'
set ylabel 'Time(nsec)'
set xtics 0,10
set style fill solid
set title 'Fast doubling and Arbitrary precision'
set term png enhanced font 'Verdana, 10'
set output 'result.png'
set format y
plot[:][:]'out' \
using 6:10 with linespoints linewidth 2 title 'Execution time'
