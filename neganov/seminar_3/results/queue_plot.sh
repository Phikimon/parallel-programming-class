#!/bin/bash
gnuplot <<< "set term png size 1920,1080; \
             set output 'queue_performance.png'; \
             set multiplot layout 1, 1 title 'Execution time(in sec) from number of threads' font \",14\"; \
             set tmargin 2; \
             set key right bottom; \
             set yrange [0:400]; \
             set xrange [1:9]; \
             set style line 1 linewidth 2 pointsize 2; \
             set style line 2 linewidth 2 pointsize 2; \
             set style line 3 linewidth 2 pointsize 2; \
             set style line 4 linewidth 2 pointsize 2; \
             set style line 5 linewidth 2 pointsize 2; \
             set style line 6 linewidth 2 pointsize 2; \
             set style line 7 linewidth 2 pointsize 2; \
             set style line 8 linewidth 2 pointsize 2; \
             set style line 9 linewidth 2 pointsize 2; \
             set style line 10 linewidth 2 pointsize 2; \
             plot 'queue_shared_1.dat'     u 1:(column(2)/1000) title 'queue shared 1'             w linespoints ls 10, \
                  'queue_shared_2.dat'     u 1:(column(2)/1000) title 'queue shared 2'             w linespoints ls 10, \
                  'queue_excl_1.dat'       u 1:(column(2)/1000) title 'queue excl 1'               w linespoints ls 6, \
                  'queue_excl_2.dat'       u 1:(column(2)/1000) title 'queue excl 2'               w linespoints ls 6, \
                  'queue_no_deref_1.dat'   u 1:(column(2)/1000) title 'queue excl no deref line 1' w linespoints ls 7, \
                  'queue_no_deref_2.dat'   u 1:(column(2)/1000) title 'queue excl no deref line 2' w linespoints ls 7, \
                  'queue_pause_1.dat'      u 1:(column(2)/1000) title 'queue pause 1'              w linespoints ls 8, \
                  'queue_pause_2.dat'      u 1:(column(2)/1000) title 'queue pause 2'              w linespoints ls 8, \
                  'queue_pause_atom_1.dat' u 1:(column(2)/1000) title 'queue pause atom 1'         w linespoints ls 9, \
                  'queue_pause_atom_2.dat' u 1:(column(2)/1000) title 'queue pause atom 2'         w linespoints ls 9;"

