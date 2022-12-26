#!bin/sh
make clean
make main
srun ./main testcases/$1.in outputs/$1.out 0 1

cd utilities
make verify
cd ..
./utilities/verify testcases/$1.in outputs/$1.out 0 1