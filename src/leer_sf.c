#include "bloques.h"
#include "ficheros_basico.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, BOLD "uso: " RESET "%s <nombre_dispositivo>\n", argv[0]);
        return FALLO;
    }

    const char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    superbloque_t sb = {};
    bread(posSB, &sb);

    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %d\n", sb.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", sb.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", sb.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", sb.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", sb.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", sb.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", sb.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", sb.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", sb.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", sb.cantInodosLibres);
    printf("totBloques = %d\n", sb.totBloques);
    printf("totInodos = %d\n", sb.totInodos);
    printf("\n");
    printf("sizeof struct superbloque: %lu\n", sizeof(superbloque_t));
    printf("sizeof struct inodo: %lu\n", sizeof(inodo_t));
    printf("\n");
    printf("DATOS DEL SUPERBLOQUE\n");

    inodo_t inodos[BLOCKSIZE / INODOSIZE] = {};
    for (unsigned int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) {
        bread(i, inodos);
        for (unsigned int j = 0; j < BLOCKSIZE / INODOSIZE; j++) printf("%d ", inodos[j].punterosDirectos[0]);
    }
    printf("\n");

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    return EXITO;
}
