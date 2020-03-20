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
	using 2:xtic(10) with linespoints linewidth 2 title 'k + 2', \
''  using 3:xtic(10) with linespoints linewidth 2 title 'fib ctz', \
''  using 4:xtic(10) with linespoints linewidth 2 title 'fast logn'\
