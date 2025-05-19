# use CACHE env variable to compile with different cache levels:
#    CACHE=0  ->  disabled
#    CACHE=1  ->  single
#    CACHE=2  ->  FIFO
#    CACHE=3  ->  LRU

# for CACHE FIFO/LRU use CACHE_SIZE to set a cache size (default value = 3)

make clean
CACHE=2 DEBUG=1 make
./build/mi_mkfs disco 100000
./build/prueba_cache_tabla disco "hola"

make clean
CACHE=3 DEBUG=1 make
./build/mi_mkfs disco 100000
./build/prueba_cache_tabla disco "hola"
