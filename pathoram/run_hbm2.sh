cd build
make -j8
cp ./ramulator2 ../ramulator2
cd ..
./ramulator2 -f ./config_oram_hbm2_ch1.yaml
./ramulator2 -f ./config_oram_hbm2_ch2.yaml
./ramulator2 -f ./config_oram_hbm2_ch4.yaml