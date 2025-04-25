/**************************************************************************
* filename: mi_stat.c
* author: serafí nebot, ignasi paredes, jaume galmés
**************************************************************************/

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> </ruta>\n", argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1];
    const char *const path = argv[2];

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    stat_t stat;
    int p_inode = mi_stat(path, &stat);

    if (p_inode > 0) {
        printf("Nº de inodo: %u\n", p_inode); 
        printf("tipo: %c\n", stat.tipo);
        printf("permisos: %hhu\n", stat.permisos);

        struct tm *ts;
        char time_str[32] = {};

        ts = localtime(&stat.atime);
        strftime(time_str, sizeof(time_str), TMSP_FMT, ts);
        printf("atime: %s\n", time_str);

        ts = localtime(&stat.mtime);
        strftime(time_str, sizeof(time_str), TMSP_FMT, ts);
        printf("mtime: %s\n", time_str);

        ts = localtime(&stat.ctime);
        strftime(time_str, sizeof(time_str), TMSP_FMT, ts);
        printf("ctime: %s\n", time_str);

        ts = localtime(&stat.btime);
        strftime(time_str, sizeof(time_str), TMSP_FMT, ts);
        printf("btime: %s\n", time_str);

        printf("nlinks: %u\n", stat.nlinks);
        printf("tamEnBytesLog: %u\n", stat.tamEnBytesLog);
        printf("numBloquesOcupados: %u\n", stat.numBloquesOcupados);
    }

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return p_inode;
}
