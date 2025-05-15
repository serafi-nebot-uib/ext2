/**************************************************************************
* FILENAME: ficheros_basico.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __FICHEROS_BASICO_H__
#define __FICHEROS_BASICO_H__

// TODO: load virtual device with mmap

#include <limits.h>
#include <time.h>

#include "bloques.h"
#include "util/colors.h"

#define posSB 0 // posición predeterminada, el superbloque se escribe en el primer bloque de nuestro FS
#define tamSB 1

#define NPUNTEROS (BLOCKSIZE / sizeof(unsigned int))   // 256 punteros por bloque
#define DIRECTOS 12
#define INDIRECTOS0 (NPUNTEROS + DIRECTOS)    // 268
#define INDIRECTOS1 (NPUNTEROS * NPUNTEROS + INDIRECTOS0)    // 65.804
#define INDIRECTOS2 (NPUNTEROS * NPUNTEROS * NPUNTEROS + INDIRECTOS1) // 16.843.020

#define INODE_PTR_LVL_MAX 3

typedef union {
    struct {
        unsigned int posPrimerBloqueMB;                         // posición absoluta del primer bloque del mapa de bits
        unsigned int posUltimoBloqueMB;                         // posición absoluta del último bloque del mapa de bits
        unsigned int posPrimerBloqueAI;                         // posición absoluta del primer bloque del array de inodos
        unsigned int posUltimoBloqueAI;                         // posición absoluta del último bloque del array de inodos
        unsigned int posPrimerBloqueDatos;                      // posición absoluta del primer bloque de datos
        unsigned int posUltimoBloqueDatos;                      // posición absoluta del último bloque de datos
        unsigned int posInodoRaiz;                              // posición del inodo del directorio raíz (relativa al AI)
        unsigned int posPrimerInodoLibre;                       // posición del primer inodo libre (relativa al AI)
        unsigned int cantBloquesLibres;                         // cantidad de bloques libres (en todo el disco)
        unsigned int cantInodosLibres;                          // cantidad de inodos libres (en el AI)
        unsigned int totBloques;                                // cantidad total de bloques del disco
        unsigned int totInodos;                                 // cantidad total de inodos (heurística)
    };
    char padding[BLOCKSIZE];                                    // relleno para ocupar el bloque completo
} superbloque_t;

#define INODOSIZE           128 // tamaño en bytes de un inodo
#define INODOS_IN_BLOCK     (BLOCKSIZE/INODOSIZE) // numero de inodos en un bloque

#define INODE_P_READ     0b100
#define INODE_P_WRITE    0b010
#define INODE_P_EXECUTE  0b001

#define INODE_P(perm, mask) (((perm) & (mask)) == (mask))

typedef union {
    struct {
        unsigned char tipo;                     // tipo ('l':libre, 'd':directorio o 'f':fichero)
        unsigned char permisos;                 // permisos (lectura y/o escritura y/o ejecución)
        time_t atime;                           // fecha y hora del último acceso a datos: atime
        time_t mtime;                           // fecha y hora de la última modificación de datos: mtime
        time_t ctime;                           // fecha y hora de la última modificación del inodo: ctime
        time_t btime;                           // fecha y hora de creación del inodo: btime (birth)
        unsigned int nlinks;                    // cantidad de enlaces de entradas en directorio
        unsigned int tamEnBytesLog;             // tamaño en bytes lógicos
        unsigned int numBloquesOcupados;        // cantidad de bloques ocupados zona de datos
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
int obtener_nRangoBL(inodo_t *inodo, unsigned int nblogico, unsigned int *ptr);
int obtener_indice(unsigned int nblogico, int nivel_punteros);
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar);
int liberar_bloques_inodo(unsigned int primerBL, inodo_t *inodo);
int liberar_inodo(unsigned int ninodo);
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes);

#endif
