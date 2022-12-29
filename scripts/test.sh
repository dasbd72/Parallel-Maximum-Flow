#!bin/sh
make clean
make main
rm -f outputs/$1.out
srun -c 12 ./main testcases/$1.in outputs/$1.out 0 1

cd tools
make verify
cd ..
./tools/verify testcases/$1.in outputs/$1.out 0 1