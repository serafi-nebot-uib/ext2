/**************************************************************************
* FILENAME: leer_sf.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

void mostrar_buscar_entrada(char *camino, char reservar){
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) mostrar_error_buscar_entrada(error);
    printf("**********************************************************************\n");
    return;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, RED "sintaxis: %s <nombre_dispositivo>\n" RESET, argv[0]);
        return FALLO;
    }

    const char *dev_name = argv[1];
    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    superbloque_t sb = {};
    if (bread(posSB, &sb) == FALLO) return FALLO;

    printf("\n");
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
    // printf("sizeof struct superbloque: %lu\n", sizeof(superbloque_t));
    // printf("sizeof struct inodo: %lu\n", sizeof(inodo_t));
    printf("\n");

    // Mostrar creación directorios y errores
    // mostrar_buscar_entrada("pruebas/", 1); //ERROR_CAMINO_INCORRECTO
    // mostrar_buscar_entrada("/pruebas/", 0); //ERROR_NO_EXISTE_ENTRADA_CONSULTA
    // mostrar_buscar_entrada("/pruebas/docs/", 1); //ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
    // mostrar_buscar_entrada("/pruebas/", 1); // creamos /pruebas/
    // mostrar_buscar_entrada("/pruebas/docs/", 1); //creamos /pruebas/docs/
    // mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //creamos /pruebas/docs/doc1
    // mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
    // //ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
    // mostrar_buscar_entrada("/pruebas/", 1); //ERROR_ENTRADA_YA_EXISTENTE
    // mostrar_buscar_entrada("/pruebas/docs/doc1", 0); //consultamos /pruebas/docs/doc1
    // mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //ERROR_ENTRADA_YA_EXISTENTE
    // mostrar_buscar_entrada("/pruebas/casos/", 1); //creamos /pruebas/casos/
    // mostrar_buscar_entrada("/pruebas/docs/doc2", 1); //creamos /pruebas/docs/doc2
    // printf("\n");

    // inodo_t inodos[BLOCKSIZE / INODOSIZE] = {};
    // for (unsigned int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) {
    //     bread(i, inodos);
    //     for (unsigned int j = 0; j < BLOCKSIZE / INODOSIZE; j++) printf("%d ", inodos[j].punterosDirectos[0]);
    // }
    // printf("\n");

    // printf("RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    // int nblock = reservar_bloque();
    // bread(posSB, &sb);
    // printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", nblock);
    // printf("SB.cantBloquesLibres: %u\n", sb.cantBloquesLibres);
    // liberar_bloque(nblock);
    // bread(posSB, &sb);
    // printf("Liberamos ese bloque y después SB.cantBloquesLibres = %u\n", sb.cantBloquesLibres);
    // printf("\n");

    // printf("MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
    // printf("posSB: %u → leer_bit(%u) = %u\n", posSB, posSB, leer_bit(posSB));
    // printf("SB.posPrimerBloqueMB: %u → leer_bit(%u) = %u\n", sb.posPrimerBloqueMB, sb.posPrimerBloqueMB, leer_bit(sb.posPrimerBloqueMB));
    // printf("SB.posUltimoBloqueMB: %u → leer_bit(%u) = %u\n", sb.posUltimoBloqueMB, sb.posUltimoBloqueMB, leer_bit(sb.posUltimoBloqueMB));
    // printf("SB.posPrimerBloqueAI: %u → leer_bit(%u) = %u\n", sb.posPrimerBloqueAI, sb.posPrimerBloqueAI, leer_bit(sb.posPrimerBloqueAI));
    // printf("SB.posUltimoBloqueAI: %u → leer_bit(%u) = %u\n", sb.posUltimoBloqueAI, sb.posUltimoBloqueAI, leer_bit(sb.posUltimoBloqueAI));
    // printf("SB.posPrimerBloqueDatos: %u → leer_bit(%u) = %u\n", sb.posPrimerBloqueDatos, sb.posPrimerBloqueDatos, leer_bit(sb.posPrimerBloqueDatos));
    // printf("SB.posUltimoBloqueDatos: %u → leer_bit(%u) = %u\n", sb.posUltimoBloqueDatos, sb.posUltimoBloqueDatos, leer_bit(sb.posUltimoBloqueDatos));
    // printf("\n");

    // inodo_t inodo = {};
    // leer_inodo(sb.posInodoRaiz, &inodo);
    // printf("DATOS DEL DIRECTORIO RAIZ\n");
    // printf("tipo: %c\n", inodo.tipo);
    // printf("permisos: %hhu\n", inodo.permisos);
    //
    // char time_str[32] = {};
    // struct tm *ts;
    //
    // ts = localtime(&inodo.atime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("atime: %s\n", time_str);
    //
    // ts = localtime(&inodo.mtime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("mtime: %s\n", time_str);
    //
    // ts = localtime(&inodo.ctime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("ctime: %s\n", time_str);
    //
    // ts = localtime(&inodo.btime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("btime: %s\n", time_str);
    //
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("nlinks: %u\n", inodo.nlinks);
    // printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);
    // printf("numBloquesOcupados: %u\n", inodo.numBloquesOcupados);


    // printf("INODO 1. TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 y 468.750\n\n");
    // unsigned int blocks[] = { 8, 204, 30004, 400004, 468750 };
    // int ninode = reservar_inodo('f', 06);
    // for (unsigned int i = 0; i < sizeof(blocks) / sizeof(*blocks); i++) {
    //     traducir_bloque_inodo(ninode, blocks[i], 1);
    //     printf("\n");
    // }

    // inodo_t inode = {};
    // leer_inodo(ninode, &inode);
    // printf("DATOS DEL INODO RESERVADO 1\n");
    // printf("tipo: %c\n", inode.tipo);
    // printf("permisos: %hhu\n", inode.permisos);

    // char time_str[32] = {};
    // struct tm *ts;

    // ts = localtime(&inode.atime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("atime: %s\n", time_str);

    // ts = localtime(&inode.mtime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("mtime: %s\n", time_str);

    // ts = localtime(&inode.ctime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("ctime: %s\n", time_str);

    // ts = localtime(&inode.btime);
    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("btime: %s\n", time_str);

    // strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
    // printf("nlinks: %u\n", inode.nlinks);
    // printf("tamEnBytesLog: %u\n", inode.tamEnBytesLog);
    // printf("numBloquesOcupados: %u\n", inode.numBloquesOcupados);

    // if (bread(posSB, &sb) == FALLO) return FALLO;
    // printf("sb.posPrimerInodoLibre: %u\n", sb.posPrimerInodoLibre);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return EXITO;
}
