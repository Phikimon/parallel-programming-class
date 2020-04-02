#!/bin/bash
set +x

make
mpirun -n 16 --oversubscribe --mca btl tcp,self --mca btl_self_eager_limit 256 --mca btl_tcp_eager_limit 256 $1
