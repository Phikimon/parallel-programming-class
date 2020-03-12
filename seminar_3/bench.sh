#!/bin/bash
TARGET="$1"
if [ -z $TARGET ]
then
	echo "Usage: ./bench.sh <implementation_name> [dumb/png]"
	echo "Example: ./bench.sh filipp_tas"
	exit
fi
echo "Benchmarking $1"

GNUPLOTTERM="$2"
if [ -z $GNUPLOTTERM ]
then
	echo "No gnuplot terminal specified, defaulting to 'dumb'"
	GNUPLOTTERM=dumb
else
	if [ $GNUPLOTTERM != "dumb" ] && [ $GNUPLOTTERM != "png" ]
	then
		echo "Unknown gnuplot terminal specified, defaulting to 'dumb'"
	fi
fi

echo "Making $1"
make build/$1 > /dev/null
if [ $? != 0 ]
then
	echo "Failed to make \"$1\""
	exit
fi

echo "# thread_num overall_exec_time_in_ms average_wait_metric_per_iteration" > build/plot.dat
work_amount=65536
for thread_num in 1 4 `seq 64 64 512` `seq 1024 512 4096` `seq 6144 2048 4096` #8192`
do
	iter_per_thread=$(($work_amount / $thread_num))
	echo "Running ./build/$1 $thread_num $iter_per_thread"
	( time ( ./build/$1 $thread_num $iter_per_thread >> build/plot.dat ) 2>&1 ) | grep real
	sleep 1 # Let previous process free all resources
	if [ $? != 0 ]
	then
		echo "./build/$1 failed with error $?, aborting"
		exit
	fi
done

echo "Plotting metrics for $1"
gnuplot <<< "set term png size 1920,1080; \
             set output 'res/$1.png'; \
             set multiplot layout 2, 1 title '${1/#_/ }' font \",14\"; \
             set tmargin 2; \
             set notitle; \
             plot 'build/plot.dat' u 1:2 title 'overall time(ms)' w points, \
                  'build/plot.dat' u 1:2 smooth acs notitle w lines; \
             plot 'build/plot.dat' u 1:3 title 'average latency(cpu ticks) per iteration' w points, \
                  'build/plot.dat' u 1:3 smooth acs notitle w lines"
echo "Plot image stored at res/$1.png"
if [ $GNUPLOTTERM = "png" ]
then
	echo "Opening image with plot"
	eog "res/$1.png"
fi

gnuplot <<< "set term dumb size `tput cols`,`tput lines`; \
             plot 'build/plot.dat' u 1:2 title 'overall time(ms)' w linespoints"
