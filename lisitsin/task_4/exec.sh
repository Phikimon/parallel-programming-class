#!/bin/bash
set +x

make $1 > /dev/null
mpirun -n 5 --oversubscribe $@

