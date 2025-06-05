# Adelaida
# borrar_recursivo.sh

#ejecutar primero el script estructura.sh para montar el árbol de directorios de ejmplo

echo -e "\x1B[38;2;17;245;120m$ ./build/mi_rm -r disco /dir3/\x1b[0m"
./build/mi_rm -r disco /dir3/
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo -e "\x1B[38;2;17;245;120m$ ./build/mi_rm -r disco /dir2/\x1b[0m"
./build/mi_rm -r disco /dir2/
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco
echo -e "\x1B[38;2;17;245;120m$ ./build/mi_rm -r disco /dir1/\x1b[0m"
./build/mi_rm -r disco /dir1/
echo -e "\x1B[38;2;17;245;120m$ ./build/leer_sf disco\x1b[0m"
./build/leer_sf disco