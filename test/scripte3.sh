# scripte3.sh

clear
make clean
make
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ ./build/mi_mkfs disco 100000\x1b[0m"
./build/mi_mkfs disco 100000
echo
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ time ./build/simulacion disco\x1b[0m"
SIMDIR=$(echo /simul_$(date +%Y%m%d%H%M%S)/)
time ./build/simulacion disco
echo
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ time ./build/verificacion disco ${SIMDIR}\x1b[0m"
time ./build/verificacion disco ${SIMDIR}
echo
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ ./build/mi_cat disco ${SIMDIR}informe.txt > res.txt\x1b[0m"
./build/mi_cat disco ${SIMDIR}informe.txt > res.txt
echo
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ ls -l res.txt\x1b[0m"
ls -l res.txt
echo
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ cat res.txt\x1b[0m"
cat res.txt
echo
echo "################################################################################"
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo