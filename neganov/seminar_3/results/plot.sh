#!/bin/bash
gnuplot <<< "set term png size 1920,1080; \
             set output 'performance.png'; \
             set multiplot layout 1, 1 title 'Execution time(in sec) from number of threads' font \",14\"; \
             set tmargin 2; \
             set key right bottom; \
             set yrange [0:400]; \
             set autoscale x; \
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
             set style line 11 linewidth 2 pointsize 2; \
             plot 'tas_1.dat'              u 1:(column(2)/1000) title 'tas'                        w linespoints ls 1, \
                  'tas_2.dat'              u 1:(column(2)/1000) notitle                            w linespoints ls 1, \
                  'tas_3.dat'              u 1:(column(2)/1000) notitle                            w linespoints ls 1, \
                  'ttas_1.dat'             u 1:(column(2)/1000) title 'ttas'                       w linespoints ls 2, \
                  'ttas_2.dat'             u 1:(column(2)/1000) notitle                            w linespoints ls 2, \
                  'ttas_pause_1.dat'       u 1:(column(2)/1000) title 'ttas pause'                 w linespoints ls 3, \
                  'ttas_pause_2.dat'       u 1:(column(2)/1000) notitle                            w linespoints ls 3, \
                  'ttas_wait_1.dat'        u 1:(column(2)/1000) title 'ttas wait'                  w linespoints ls 4, \
                  'ttas_wait_2.dat'        u 1:(column(2)/1000) notitle                            w linespoints ls 4, \
                  'ticket_1.dat'           u 1:(column(2)/1000) title 'ticket'                     w linespoints ls 11, \
                  'ticket_2.dat'           u 1:(column(2)/1000) notitle                            w linespoints ls 11, \
                  'queue_shared_1.dat'     u 1:(column(2)/1000) title 'queue shared'               w linespoints ls 10, \
                  'queue_shared_2.dat'     u 1:(column(2)/1000) notitle                            w linespoints ls 10, \
                  'queue_excl_1.dat'       u 1:(column(2)/1000) title 'queue excl'                 w linespoints ls 6, \
                  'queue_excl_2.dat'       u 1:(column(2)/1000) notitle                            w linespoints ls 6, \
                  'queue_no_deref_1.dat'   u 1:(column(2)/1000) title 'queue excl no deref'        w linespoints ls 7, \
                  'queue_no_deref_2.dat'   u 1:(column(2)/1000) notitle                            w linespoints ls 7, \
                  'queue_pause_1.dat'      u 1:(column(2)/1000) title 'queue pause'                w linespoints ls 8, \
                  'queue_pause_2.dat'      u 1:(column(2)/1000) notitle                            w linespoints ls 8, \
                  'queue_pause_atom_1.dat' u 1:(column(2)/1000) title 'queue pause atom'           w linespoints ls 9, \
                  'queue_pause_atom_2.dat' u 1:(column(2)/1000) notitle                            w linespoints ls 9;"

