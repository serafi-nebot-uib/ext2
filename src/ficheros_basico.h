#ifndef __FICHEROS_BASICO_H__
#define __FICHEROS_BASICO_H__

// TODO: load virtual device with mmap

#include "bloques.h"
#include <limits.h>

#define posSB 0 // el superbloque se escribe en el primer bloque de nuestro FS
#define tamSB 1

typedef union {
    struct {
        unsigned int posPrimerBloqueMB;                         // Posición absoluta del primer bloque del mapa de bits
        unsigned int posUltimoBloqueMB;                         // Posición absoluta del último bloque del mapa de bits
        unsigned int posPrimerBloqueAI;                         // Posición absoluta del primer bloque del array de inodos
        unsigned int posUltimoBloqueAI;                         // Posición absoluta del último bloque del array de inodos
        unsigned int posPrimerBloqueDatos;                      // Posición absoluta del primer bloque de datos
        unsigned int posUltimoBloqueDatos;                      // Posición absoluta del último bloque de datos
        unsigned int posInodoRaiz;                              // Posición del inodo del directorio raíz (relativa al AI)
        unsigned int posPrimerInodoLibre;                       // Posición del primer inodo libre (relativa al AI)
        unsigned int cantBloquesLibres;                         // Cantidad de bloques libres (en todo el disco)
        unsigned int cantInodosLibres;                          // Cantidad de inodos libres (en el AI)
        unsigned int totBloques;                                // Cantidad total de bloques del disco
        unsigned int totInodos;                                 // Cantidad total de inodos (heurística)
    };
    char padding[BLOCKSIZE];                                    // Relleno para ocupar el bloque completo
} superbloque_t;

#define INODOSIZE           128 // tamaño en bytes de un inodo
#define INODOS_IN_BLOCK     (BLOCKSIZE/INODOSIZE) // numero de inodos en un bloque

typedef union {
    struct {
        unsigned char tipo;                     // Tipo ('l':libre, 'd':directorio o 'f':fichero)
        unsigned char permisos;                 // Permisos (lectura y/o escritura y/o ejecución)
        time_t atime;                           // Fecha y hora del último acceso a datos: atime
        time_t mtime;                           // Fecha y hora de la última modificación de datos: mtime
        time_t ctime;                           // Fecha y hora de la última modificación del inodo: ctime
        time_t btime;                           // Fecha y hora de creación del inodo: btime (birth)
        unsigned int nlinks;                    // Cantidad de enlaces de entradas en directorio
        unsigned int tamEnBytesLog;             // Tamaño en bytes lógicos
        unsigned int numBloquesOcupados;        // Cantidad de bloques ocupados zona de datos
        unsigned int punterosDirectos[12];      // 12 punteros directos
        unsigned int punterosIndirectos[3];     // 3 punteros indirectos: 1 simple, 1 doble, 1 triple
    };
    char padding[INODOSIZE]; // padding extra para que cada inodo ocupe exactamente 128 bytes
} inodo_t;

int tamMB(unsigned int nbloques);
int tamAI(unsigned int ninodos);
int initSB(unsigned int nbloques, unsigned int ninodos);
int initMB();
int initAI();
int escribir_bit(unsigned int nbloque, unsigned int bit);
int leer_bit(unsigned int nbloque);
int reservar_bloque();
int liberar_bloque(unsigned int nbloque);
int escribir_inodo(unsigned int ninodo, inodo_t *inodo);
int leer_inodo(unsigned int ninodo, inodo_t *inodo);
int reservar_inodo(unsigned char tipo, unsigned char permisos);

#endif
