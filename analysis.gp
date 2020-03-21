reset

set ylabel 'time(nsec)'
set xlabel 'number'
set xtics 0,10
set style fill solid
set title 'Fibonacci sequence performance'
set term png enhanced font 'Verdana,10'
set output 'performance.png'
set key left top
set format y

plot [:][:]'performance' \
	using 2:xtic(10) with linespoints linewidth 2 title 'fib clzctz', \
''  using 3:xtic(10) with linespoints linewidth 2 title 'fib fast doubling', \
#''  using 4:xtic(10) with linespoints linewidth 2 title 'fib 3'\
