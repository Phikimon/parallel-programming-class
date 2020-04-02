#!/bin/bash
TARGET="$1"
if [ -z $TARGET ]
then
	echo "Usage: ./bench.sh <implementation_name>"
	echo "Example: ./bench.sh filipp_tas"
	exit
fi
echo "=====> $1"

echo "Making $1"
make build/$1 > /dev/null
if [ $? != 0 ]
then
	echo "Failed to make \"$1\""
	exit
fi

echo "# thread_num overall_exec_time_in_ms average_wait_metric_per_iteration" > build/plot.dat
work_amount=134217728
for thread_num in 1 4 6 7 8
do
	iter_per_thread=$(($work_amount / $thread_num))
	echo "Running ./build/$1 $thread_num $iter_per_thread"
	TIME_SEC=`date "+%s"`
	./build/$1 $thread_num $iter_per_thread >> build/plot.dat
	EXCODE="$?"
	printf "\t$((`date "+%s"`-$TIME_SEC)) sec\n"
	sleep 1 # Let previous process free all resources
	if [[ $EXCODE != 0 ]]
	then
		echo "./build/$1 failed with error $?, aborting"
		exit
	fi
done

echo "Plotting metrics for $1"
mkdir res/ 2> /dev/null
gnuplot <<< "set term png size 1920,1080; \
             set output 'res/$1.png'; \
             set multiplot layout 2, 1 title '${1/#_/ }' font \",14\"; \
             set tmargin 2; \
             set notitle; \
             plot 'build/plot.dat' u 1:2 title 'overall time(ms)' w linespoints; \
             plot 'build/plot.dat' u 1:3 title 'average latency(cpu ticks) per iteration' w linespoints"

gnuplot <<< "set term dumb size `tput cols`,`tput lines`; \
             plot 'build/plot.dat' u 1:2 title 'overall time(ms)' w linespoints"

FILENAME="plot_data/$1_`date "+%s"`.dat"
echo "Saving plot data in $FILENAME"
mkdir plot_data/ 2>/dev/null
cp build/plot.dat $FILENAME
