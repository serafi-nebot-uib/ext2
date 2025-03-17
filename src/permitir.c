#include "ficheros.h"

int main(int argc, char **argv) {
    if (argc != 4 || argv[2][0] == '-') { //Aseguramos que es un ninodo positivo
        fprintf(stderr, BOLD "Sintaxis: " RESET "%s <nombre_dispositivo> <ninodo> <permisos>\n", argv[0]);
        return FALLO;
    }

    const char * nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned char permisos = argv[3][0];

    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    if(mi_chmod_f(ninodo, permisos) == FALLO) return FALLO;

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    return 0;
}
