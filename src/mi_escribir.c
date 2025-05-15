/**************************************************************************
* FILENAME: mi_escribir.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> </ruta_fichero> <texto> <offset>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta = argv[2];
    char *texto = argv[3];
    unsigned int offset = (unsigned int) strtoul(argv[4], NULL, 10);

    DEBUG(1, "nombre_dispositivo: %s", nombre_dispositivo);
    DEBUG(1, "ruta: %s", ruta);
    DEBUG(1, "texto: %s", texto);
    DEBUG(1, "offset: %u", offset);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    int ret = EXITO;
    stat_t stat;
    if (mi_stat(ruta, &stat) == FALLO) {
        ERROR("no se ha podido leer \"%s\"", ruta);
        ret = FALLO;
    } else {
        if (stat.tipo != 'f') {
            ERROR("\"%s\" no es un fichero", ruta);
            ret = FALLO;
        } else {
            int nbytes = mi_write(ruta, texto, offset, strlen(texto));
            if (nbytes < 0) {
                ret = FALLO;
                nbytes = 0;
            }
            printf("longitud texto: %ld \n", strlen(texto));
            printf("Bytes escritos: %d \n\n", nbytes);
        }
    }

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: %s", nombre_dispositivo);
        return FALLO;
    }

    return ret;
}
