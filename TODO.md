cmd:
    ls -l ext1.txt #comprobamos el tamaño del fichero externo

expected:
    -rw-rw-r-- 1 uib uib 3751 feb 12 15:24 ext1.txt

actual:
    -rw-r--r--  1 hexdhog  staff  3770 May 19 11:16 ext1.txt

file size is not correct
