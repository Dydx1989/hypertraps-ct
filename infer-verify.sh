# generate synthetic datasets
mkdir VerifyData/
cd Verify
gcc -o3 generate-easycube.c -lm -o generate-easycube.ce
gcc -o3 generate-hardcube.c -lm -o generate-hardcube.ce
gcc -o3 generate-cross.c -lm -o generate-cross.ce
./generate-easycube.ce
./generate-hardcube.ce
./generate-cross.ce
cp synth* ../VerifyData

cd ../Inference

# compile and run hypertraps code
gcc -o3 hypertraps-all.c -lm -o hypertraps-all.ce
./hypertraps-all.ce ../VerifyData/synth-cross-samples-0.txt ../VerifyData/synth-cross-times-0.txt ../VerifyData/synth-cross-times-0.txt 1 2 5 1 0 > ../VerifyData/cross03125.tmp &
./hypertraps-all.ce ../VerifyData/synth-cross-samples-1.txt ../VerifyData/synth-cross-times-1.txt ../VerifyData/synth-cross-times-1.txt 1 2 5 1 0 > ../VerifyData/cross13125.tmp &
./hypertraps-all.ce ../VerifyData/synth-cross-samples-2.txt ../VerifyData/synth-cross-times-2.txt ../VerifyData/synth-cross-times-2.txt 1 2 5 1 0 > ../VerifyData/cross23125.tmp &
./hypertraps-all.ce ../VerifyData/synth-hardcube-data.txt ../VerifyData/synth-hardcube-time.txt ../VerifyData/synth-hardcube-time.txt 1 2 5 1 0 > ../VerifyData/hardcube3125.tmp &
./hypertraps-all.ce ../VerifyData/synth-easycube-data.txt ../VerifyData/synth-easycube-time.txt ../VerifyData/synth-easycube-time.txt 1 2 5 1 0 > ../VerifyData/easycube3125.tmp &
