/**************************************************************************
* FILENAME: mi_ls.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include <stdbool.h>
#include "util/colors.h"
#include "core/directorios.h"

void syntax(const char *const name) {
    fprintf(stderr, BOLD "sintaxis: " RESET "%1$s <disco> </ruta>\n\t%1$s -l <disco> </ruta>\n", name);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        syntax(argv[0]);
        return FALLO;
    }

    const bool l = strcmp(argv[1], "-l") == 0;

    if (l && argc < 4) {
        syntax(argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1 + l];
    const char *const path = argv[2 + l];

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    char str[TAMBUFFER] = { 0 };
    int ret = mi_dir(path, str, l);
    if (ret < 0) mostrar_error_buscar_entrada(ret);
    else printf("%s", str);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return ret >= 0 ? EXITO : FALLO;
}
