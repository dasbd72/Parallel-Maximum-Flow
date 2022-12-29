cd tools
make generate
cd ..

srun ./tools/generate $1 testcases/$2
