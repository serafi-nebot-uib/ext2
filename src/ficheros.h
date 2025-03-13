/**************************************************************************
* FILENAME: ficheros.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __FICHEROS_H__
#define __FICHEROS_H__

#include "ficheros_basico.h"

#include <time.h>

typedef struct STAT {
    unsigned char tipo;                     // tipo ('l':libre, 'd':directorio o 'f':fichero)
    unsigned char permisos;                 // permisos (lectura y/o escritura y/o ejecución)
    time_t atime;                           // fecha y hora del último acceso a datos: atime
    time_t mtime;                           // fecha y hora de la última modificación de datos: mtime
    time_t ctime;                           // fecha y hora de la última modificación del inodo: ctime
    time_t btime;                           // fecha y hora de creación del inodo: btime (birth)
    unsigned int nlinks;                    // cantidad de enlaces de entradas en directorio
    unsigned int tamEnBytesLog;             // tamaño en bytes lógicos
    unsigned int numBloquesOcupados;        // cantidad de bloques ocupados zona de datos
} stat_t;

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_stat_f(unsigned int ninodo, stat_t *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);

#endif
