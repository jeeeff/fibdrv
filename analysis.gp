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

plot [:100][:]'performance' \
	using 2:xtic(10) with linespoints linewidth 2 title 'Arbitrary precision arithmetic', \
''  using 3:xtic(10) with linespoints linewidth 2 title 'Fast fib sequence without clz', \
''  using 4:xtic(10) with linespoints linewidth 2 title 'Fast doubling'
