#!bin/sh
make main
rm -f outputs/$1.out
srun -c 12 ./main testcases/$1.in outputs/$1.out

cd tools
make verify
cd ..
srun ./tools/verify testcases/$1.in outputs/$1.out