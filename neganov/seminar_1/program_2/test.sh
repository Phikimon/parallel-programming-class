#!/bin/bash

NTHREADS=9
NCHARS=1000

make > /dev/null
./main
for i in `seq 1 $NTHREADS`
do
	test -e "$i.txt"
	if [ $? != "0" ]
	then
		echo "Basic test not passed: file $i.txt nonexistent"
		exit
	fi
	if [ `grep -v "$i" "$i.txt"` ]
	then
		echo "Basic test not passed: file $i.txt:"
		grep -v '$i' "$i.txt"
		exit
	fi
	if [ "a""`wc -c "$i.txt" | awk '{print $1}'`" != "a"$NCHARS ]
	then
		echo "Basic test not passed: file $i.txt:"
		wc -c "$i.txt"
		exit
	fi
done
echo "Basic test passed"
make clean > /dev/null
