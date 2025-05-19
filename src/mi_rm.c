/**************************************************************************
* FILENAME: mi_rm.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include <stdbool.h>
#include "core/directorios.h"

void syntax(const char *const name) {
    fprintf(stderr, RED "sintaxis: %s <disco> </ruta>\n" RESET, name);
}

int rm_function(const char *ruta, char r){
    int rtrn;
    char str[TAMBUFFER] = { 0 };
    int n = mi_dir(ruta, str, 0);
    if (n < 0) return FALLO;

    if (r == false || n == 0) { // Entra si r == falso o si no hay entradas en el directorio
        rtrn = mi_unlink(ruta);
    } else if (n > 0) { // entra si r == true y n > 0
        char *aux = strchr(str, '\n');
        if (aux != NULL) {
            aux++;
            char *fileName[n];
            int i = 0;
            char *token = strtok(aux, "\t");
            while (token != NULL && i < n) {
                fileName[i++] = token;
                token = strtok(NULL, "\t");
            }

            for (int j = 0; j < i; j++) {
                printf("%s\n", fileName[j]);
                char *rutaFicheroActual;
                if (ruta[strlen(ruta) - 1] != '/') rutaFicheroActual = ruta + "/" + fileName[j];
                else rutaFicheroActual = ruta + fileName[j];
                rm_function(rutaFicheroActual, 0); // Llamamos a la propia función en modo eliminar fichero
            }
        }
    }
    return rtrn;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        syntax(argv[0]);
        return FALLO;
    }

    const bool r = strcmp(argv[1], "-r") == 0;

    if (r && argc < 4) {
        syntax(argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1 + r];
    char *ruta = argv[2 + r];

    DEBUG(2, "nombre_dispositivo: %s", nombre_dispositivo);
    DEBUG(2, "ruta: %s", ruta);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    int ret = rm_function(ruta, r);
    if (ret < 0) mostrar_error_buscar_entrada(ret);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: %s", nombre_dispositivo);
        return FALLO;
    }

    return ret;
}
