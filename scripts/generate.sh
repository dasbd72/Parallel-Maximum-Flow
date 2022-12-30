cd tools
make generate
cd ..

srun ./tools/generate $1 $2 testcases/$3
