#!/bin/bash
N=128

make
rm -f plot.dat

for B in 1 2 4 8 16
do
	if (($B <= 8)); then
		echo -n "$B `bc <<< "$N*$N*$N/$B"` " >> plot.dat
	else
		echo -n "$B `bc <<< "$N*$N*$N/8"` " >> plot.dat
	fi
	perf stat -e L1-dcache-load-misses ./main <<< "$N $B" 2>&1 \
		| grep L1 \
		| awk '{print $1}'  \
		| awk 'BEGIN { FS = "." }; {print $1}' \
		| sed 's/,//g' >> plot.dat
done

gnuplot <<< "set term dumb 140 50;\
             plot 'plot.dat' u 1:3 title 'real'      w l,\
                  'plot.dat' u 1:2 title 'predicted' w l"
