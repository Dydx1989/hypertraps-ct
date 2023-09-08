cd Inference
gcc -o3 hypertraps-all.c -lm -o hypertraps-all.ce

# discrete time approaches
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --seed 1 --length 5 --kernel 4 > ../Data/tb-dt-1.tmp &
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --seed 2 --length 5 --kernel 4 > ../Data/tb-dt-2.tmp &

# continuous time approaches
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --times ../Data/ng.2878-S2.txt-cooked.txt-datatime.txt --seed 1 --length 6 --kernel 4 > ../Data/tb-ct-1.tmp &
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --times ../Data/ng.2878-S2.txt-cooked.txt-datatime.txt --seed 2 --length 6 --kernel 4 > ../Data/tb-ct-2.tmp &
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --times ../Data/ng.2878-S2.txt-cooked.txt-datatime.txt --seed 1 --length 7 --kernel 4 > ../Data/tb-ct-a-1.tmp &
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --times ../Data/ng.2878-S2.txt-cooked.txt-datatime.txt --seed 2 --length 7 --kernel 4 > ../Data/tb-ct-a-2.tmp &
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --times ../Data/ng.2878-S2.txt-cooked.txt-datatime.txt --seed 1 --length 6 --kernel 5 > ../Data/tb-ct-b-1.tmp &
./hypertraps-all.ce --obs ../Data/ng.2878-S2.txt-cooked.txt-data.txt --times ../Data/ng.2878-S2.txt-cooked.txt-datatime.txt --seed 2 --length 6 --kernel 5 > ../Data/tb-ct-b-2.tmp &
