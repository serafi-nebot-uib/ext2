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
    if (bread(posSB, &sb) == FALLO) return FALLO;

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

    // inodo_t inodos[BLOCKSIZE / INODOSIZE] = {};
    // for (unsigned int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) {
    //     bread(i, inodos);
    //     for (unsigned int j = 0; j < BLOCKSIZE / INODOSIZE; j++) printf("%d ", inodos[j].punterosDirectos[0]);
    // }
    // printf("\n");

    printf("RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    int nblock = reservar_bloque();
    bread(posSB, &sb);
    printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", nblock);
    printf("SB.cantBloquesLibres: %u\n", sb.cantBloquesLibres);
    liberar_bloque(nblock);
    bread(posSB, &sb);
    printf("Liberamos ese bloque y después SB.cantBloquesLibres = %u\n", sb.cantBloquesLibres);
    printf("\n");

    printf("posSB: %u → leer_bit(%u) = %u\n", posSB, posSB, leer_bit(posSB));
    printf("SB.posPrimerBloqueMB: %u → leer_bit(%u) = %u\n", sb.posPrimerBloqueMB, sb.posPrimerBloqueMB, leer_bit(sb.posPrimerBloqueMB));
    printf("SB.posUltimoBloqueMB: %u → leer_bit(%u) = %u\n", sb.posUltimoBloqueMB, sb.posUltimoBloqueMB, leer_bit(sb.posUltimoBloqueMB));
    printf("SB.posPrimerBloqueAI: %u → leer_bit(%u) = %u\n", sb.posPrimerBloqueAI, sb.posPrimerBloqueAI, leer_bit(sb.posPrimerBloqueAI));
    printf("SB.posUltimoBloqueAI: %u → leer_bit(%u) = %u\n", sb.posUltimoBloqueAI, sb.posUltimoBloqueAI, leer_bit(sb.posUltimoBloqueAI));
    printf("SB.posPrimerBloqueDatos: %u → leer_bit(%u) = %u\n", sb.posPrimerBloqueDatos, sb.posPrimerBloqueDatos, leer_bit(sb.posPrimerBloqueDatos));
    printf("SB.posUltimoBloqueDatos: %u → leer_bit(%u) = %u\n", sb.posUltimoBloqueDatos, sb.posUltimoBloqueDatos, leer_bit(sb.posUltimoBloqueDatos));
    printf("\n");


    inodo_t inodo = {};
    leer_inodo(sb.posInodoRaiz, &inodo);
    printf("DATOS DEL DIRECTORIO RAIZ\n");
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %hhu\n", inodo.permisos);

    char time_str[32] = {};
    struct tm *ts;

    ts = localtime(&inodo.atime);
    strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("atime: %s\n", time_str);

    ts = localtime(&inodo.mtime);
    strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("mtime: %s\n", time_str);

    ts = localtime(&inodo.ctime);
    strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("ctime: %s\n", time_str);

    ts = localtime(&inodo.btime);
    strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("btime: %s\n", time_str);

    strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("nlinks: %u\n", inodo.nlinks);
    printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", inodo.numBloquesOcupados);

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    return EXITO;
}
