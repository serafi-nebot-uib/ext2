#include "ficheros.h"
#include "ficheros_basico.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "sintaxis: %s <nombre_dispositivo> <ninodo> <nbytes>\n", argv[0]);
        return 1;
    }

    const char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return 1;
    }

    int liberados;
    if (nbytes == 0) {
        // si nbytes es 0, se libera el inodo completo
        liberados = liberar_inodo(ninodo);
        if (liberados == FALLO) {
            ERROR("no se ha podido liberar el inodo: %u", ninodo);
            return 1;
        } else {
            printf("inodo %u liberado correctamente.\n", ninodo);
        }
    } else {
        // si nbytes > 0, se trunca el fichero a nbytes
        liberados = mi_truncar_f(ninodo, nbytes);
        if (liberados == FALLO) {
            ERROR("no se ha podido truncar el inodo: %u", ninodo);
            return 1;
        } else {
            printf("fichero truncado correctamente; bloques liberados: %d\n", liberados);
        }
    }

    stat_t stat;
    if (mi_stat_f(ninodo, &stat) == FALLO) {
        ERROR("no se ha podido leer el stat del inodo: %u", ninodo);
    } else {
        printf("DATOS INODO: %u\n", ninodo);
        printf("tipo: %c\n", stat.tipo);
        printf("permisos: %hhu\n", stat.permisos);

        struct tm *ts;
        char time_str[32] = {};

        ts = localtime(&stat.atime);
        strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
        printf("atime: %s\n", time_str);

        ts = localtime(&stat.mtime);
        strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
        printf("mtime: %s\n", time_str);

        ts = localtime(&stat.ctime);
        strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
        printf("ctime: %s\n", time_str);

        ts = localtime(&stat.btime);
        strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", ts);
        printf("btime: %s\n", time_str);

        printf("nlinks: %u\n", stat.nlinks);
        printf("tamEnBytesLog: %u\n", stat.tamEnBytesLog);
        printf("numBloquesOcupados: %u\n", stat.numBloquesOcupados);
    }


    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: \"%s\"", nombre_dispositivo);
        return 1;
    }

    return 0;
}
