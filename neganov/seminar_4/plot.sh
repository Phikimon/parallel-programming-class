#!/bin/bash

PERF_CACHE_MISS_EVENT_NAME=-L1-dcache-load-misses

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
	perf stat -e $PERF_CACHE_MISS_EVENT_NAME ./main <<< "$N $B" 2>&1 \
		| grep L1 \
		| awk '{print $1}'  \
		| awk 'BEGIN { FS = "." }; {print $1}' \
		| sed 's/,//g' >> plot.dat
done

WIDTH=`tput cols`
HEIGHT=`tput rows`
if [[ $? != 0 ]]
then
	WIDTH=80
	HEIGHT=25
fi
gnuplot <<< "set term dumb $WIDTH $HEIGHT;\
             plot 'plot.dat' u 1:3 title 'real'      w l,\
                  'plot.dat' u 1:2 title 'predicted' w l"
