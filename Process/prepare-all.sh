chmod +x *.sh

./cook-data.sh ../Data/ng.2878-S2.txt ../Data/tuberculosis-v5-header-19-29.csv 1000
./cook-data.sh ../Data/mro-ncbi-tree.phy ../Data/mro-barcodes.csv 0.1
./mro-timetree-parse.sh