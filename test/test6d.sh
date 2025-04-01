# Adelaida
# test6d.sh  #truncados parciales texto varios bloques

clear
make clean
make
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./build/mi_mkfs disco 100000\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#inicializamos el sistema de ficheros con 100.000 bloques\x1b[0m"
./build/mi_mkfs disco 100000
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./build/escribir disco "$(cat ./test/texto2.txt)" 0\x1b[0m"
./build/escribir disco "$(cat ./test/texto2.txt)" 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./build/truncar disco 1 409605001\x1b[0m"
time ./build/truncar disco 1 409605001
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./build/truncar disco 1 30725003\x1b[0m"
time ./build/truncar disco 1 30725003
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./build/truncar disco 1 209008\x1b[0m"
time ./build/truncar disco 1 209008
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./build/truncar disco 1 9005\x1b[0m"
time ./build/truncar disco 1 9005
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./build/truncar disco 1 0\x1b[0m"
time ./build/truncar disco 1 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
