/**************************************************************************
* FILENAME: simulacion.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __SIMULACION_H__
#define __SIMULACION_H__

#include <stdio.h>
#include <stdint.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define REGMAX 500000
#define NUMPROCESOS 100
#define NUMESCRITURAS 50

typedef struct { // sizeof(struct REGISTRO): 24 bytes
    struct timeval fecha;
    pid_t pid; // PID del proceso que lo ha creado
    int nEscritura; // Entero con el nº de escritura, de 1 a 50 (orden por tiempo)
    int nRegistro; // Entero con el nº del registro dentro del fichero: [0..REGMAX-1] (orden por posición)
} registro_t;

#endif
