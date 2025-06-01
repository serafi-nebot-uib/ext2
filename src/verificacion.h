#ifndef __VERIFICACION_H__
#define __VERIFICACION_H__

#include <stdio.h>
#include <stdint.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "simulacion.h"

typedef struct {
    int pid;
    unsigned int nEscrituras; // validadas
    registro_t PrimeraEscritura;
    registro_t UltimaEscritura;
    registro_t MenorPosicion;
    registro_t MayorPosicion;
} informacion_t;

#endif
