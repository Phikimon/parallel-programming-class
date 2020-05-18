#!/bin/bash
set +x

make $1
mpirun -n 5 --oversubscribe --allow-run-as-root $@
