#!/bin/bash
set +x

make
mpirun -n 4 --oversubscribe $1

