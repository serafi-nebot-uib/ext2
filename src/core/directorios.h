/**************************************************************************
* FILENAME: directorios.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __DIRECTORIOS_H__
#define __DIRECTORIOS_H__

#include "ficheros.h"

#define DELIM "/"
#define TAMNOMBRE 60 // tamaño del nombre de directorio o fichero, en Ext2 = 256
#define PROFUNDIDAD 32 // profundidad máxima del árbol de directorios
#define TAMFILA 100
#define TAMBUFFER (TAMFILA * 1000) // suponemos un máx de 1000 entradas, aunque debería ser SB.totInodos

#define ERROR_CAMINO_INCORRECTO (-2)
#define ERROR_PERMISO_LECTURA (-3)
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4)
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5)
#define ERROR_PERMISO_ESCRITURA (-6)
#define ERROR_ENTRADA_YA_EXISTENTE (-7)
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8)

typedef struct  {
    char nombre[TAMNOMBRE]; //Este campo nombre no incluye la ruta (ni el carácter de separación '/').
    unsigned int ninodo;
} entrada_t;

#define ENTRADAS_IN_BLOCK (BLOCKSIZE / sizeof(entrada_t))

#ifndef CACHE
#define CACHE 1 // 0: sin caché, 1: última L/E, 2: tabla FIFO, 3: tabla LRU
#endif

#if CACHE > 0
typedef struct entrada_cache {
    char camino[TAMNOMBRE * PROFUNDIDAD]; // ruta de la entrada
    unsigned int p_inodo; // inodo de la entrada
#if CACHE == 3
    // ultima_consulta solo se necesita para LRU
    struct timeval ultima_consulta;
#endif
} entrada_cache_t;

#if CACHE == 1
// sólo se crea una entrada cache
static entrada_cache_t entrada_cache = {};
#elif CACHE > 1
    #ifndef CACHE_SIZE
        #define CACHE_SIZE 3
    #else
        #if CACHE_SIZE == 0
            #error "CACHE_SIZE debe ser > 0"
        #endif
    #endif
static unsigned int entrada_cache_top = 0; // índice del primer elemento de la cache (diferente para FIFO y LRU)
static entrada_cache_t entrada_cache[CACHE_SIZE] = {}; // array de entradas cache con CACHE_SIZE
#endif
#endif

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
void mostrar_error_buscar_entrada(int error);
int mi_creat(const char *camino, unsigned char permisos);
int mi_dir(const char *camino, char *buffer, char flag);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, stat_t *p_stat);
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);
int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);

#endif
