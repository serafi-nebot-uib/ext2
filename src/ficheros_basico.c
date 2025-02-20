#include "ficheros_basico.h"


int tamMB(unsigned int nbloques) {
    int res = nbloques / 8;
    return (res / BLOCKSIZE) + ((res % BLOCKSIZE) != 0);
}

int tamAI(unsigned int ninodos) {
    int res = (ninodos * INODOSIZE);
    return (res / BLOCKSIZE) + ((res % BLOCKSIZE) != 0);
}

int initSB(unsigned int nbloques, unsigned int ninodos) {
    superbloque_t sb = {};
    sb.posPrimerBloqueMB = posSB + tamSB;
    sb.posUltimoBloqueMB = sb.posPrimerBloqueMB + tamMB(nbloques) - 1;
    sb.posPrimerBloqueAI = sb.posUltimoBloqueMB + 1;
    sb.posUltimoBloqueAI = sb.posPrimerBloqueAI + tamAI(ninodos) - 1;
    sb.posPrimerBloqueDatos = sb.posUltimoBloqueAI + 1;
    sb.posUltimoBloqueDatos = nbloques - 1;
    sb.posInodoRaiz = 0;
    sb.posPrimerInodoLibre = 0;
    sb.cantBloquesLibres = nbloques;
    sb.cantInodosLibres = ninodos;
    sb.totBloques = nbloques;
    sb.totInodos = ninodos;
    return bwrite(posSB, &sb);
}

int initMB() {
    return 0;
}

// int initAI();
