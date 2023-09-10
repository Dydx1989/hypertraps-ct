cd Inference
gcc -o3 hypertraps-all.c -lm -o hypertraps-all.ce

### NCBI, discrete time
./hypertraps-all.ce --obs ../Data/mro-ncbi-tree.phy-cooked.txt-data.txt --seed 1 --length 5 --kernel 7 --losses --label ../Data/mro-1 > ../Data/mro-expt-1.txt &
./hypertraps-all.ce --obs ../Data/mro-ncbi-tree.phy-cooked.txt-data.txt --seed 2 --length 5 --kernel 7 --losses --label ../Data/mro-2 > ../Data/mro-expt-2.txt &

### TimeTree, discrete time
./hypertraps-all.ce --obs ../Data/mro-tree-tt-format.phy-data.txt --seed 1 --length 5 --kernel 7 --losses --label ../Data/mro-3 > ../Data/mro-expt-3.txt &
./hypertraps-all.ce --obs ../Data/mro-tree-tt-format.phy-data.txt --seed 2 --length 5 --kernel 7 --losses --label ../Data/mro-4 > ../Data/mro-expt-4.txt &

### TimeTree, continuous time
./hypertraps-all.ce --obs ../Data/mro-tree-tt-format.phy-data.txt --times ../Data/mro-tree-tt-format.phy-datatime.txt --seed 1 --length 6 --kernel 7 --losses --label ../Data/mro-5 > ../Data/mro-expt-5.txt &
./hypertraps-all.ce --obs ../Data/mro-tree-tt-format.phy-data.txt --times ../Data/mro-tree-tt-format.phy-datatime.txt --seed 2 --length 6 --kernel 7 --losses --label ../Data/mro-6 > ../Data/mro-expt-6.txt &

### TimeTree+, discrete time
./hypertraps-all.ce --obs ../Data/mro-tree-ttplus-format.phy-data.txt --seed 1 --length 5 --kernel 7 --losses --label ../Data/mro-7 > ../Data/mro-expt-7.txt &
./hypertraps-all.ce --obs ../Data/mro-tree-ttplus-format.phy-data.txt --seed 2 --length 5 --kernel 7 --losses --label ../Data/mro-8 > ../Data/mro-expt-8.txt &

### TimeTree+, continuous time
./hypertraps-all.ce --obs ../Data/mro-tree-ttplus-format.phy-data.txt --times ../Data/mro-ttplus-1.txt --endtimes ../Data/mro-ttplus-2.txt --seed 1 --length 6 --kernel 7 --losses --label ../Data/mro-9 > ../Data/mro-expt-9.txt &
./hypertraps-all.ce --obs ../Data/mro-tree-ttplus-format.phy-data.txt --times ../Data/mro-ttplus-1.txt --endtimes ../Data/mro-ttplus-2.txt --seed 2 --length 6 --kernel 7 --losses --label ../Data/mro-10 > ../Data/mro-expt-10.txt &
