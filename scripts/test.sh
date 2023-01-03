#!bin/sh
make clean
make -j 12
srun -c 12 ./main $1 $2