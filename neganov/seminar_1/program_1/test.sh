#!/bin/bash

make > /dev/null
./main 1000 | awk '{print $1}' > output_1000
seq 1 1000 > seq_1000
if [ "`cmp seq_1000 output_1000`" ]
then
	echo "Basic test not passed:"
	cmp seq_1000 output_1000
else
	echo "Basic test passed"
	make clean > /dev/null
fi
