/**************************************************************************
* FILENAME: mi_rm.c
* DESCRIPTION: Contiene la definición de mi_rm, función para eliminar
               un fichero o directorio, de forma adicional se ha añadido
               el uso del modificador "-r" que permite eliminar
               un directorio que no esté vacío, así como todo su contenido.
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include <stdbool.h>
#include "core/directorios.h"

void syntax(const char *const name) {
    fprintf(stderr, RED "sintaxis: %1$s <disco> </ruta>\n\t  %1$s -r <disco> </ruta>\n" RESET, name);
}

int rm_function(const char *ruta, const bool r) {
    int rtrn = 0;
    char str[TAMBUFFER] = { 0 };
    int n = mi_dir(ruta, str, 0);
    if (n < 0) return FALLO;
    // printf(GRAY "rm_function(parameters):\n");
    // printf("· ruta    : %s\n", ruta);
    // printf("· flag r  : %d\n", r);
    // printf("________________________\n\n" RESET);
    // printf("rm_function() -> n: %d\n\n", n);

    if (r == false || n == 0) { // Entra si r == falso o si no hay entradas en el directorio
        // printf("rm_function() -> ejecutando mi_rm normal (r == 0 || n == 0)\n");
        rtrn = mi_unlink(ruta);
    } else if (n > 0) { // entra si r == true y n > 0
        // printf("rm_function() -> ejecutando mi_rm -r (r == 1 && n > 0)\n");

        superbloque_t sb;
        if (bread(posSB, &sb) == FALLO) return FALLO;

        unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
        int rtrn = buscar_entrada(ruta, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (rtrn < 0) return rtrn;

        inodo_t inode;
        if (leer_inodo(p_inodo, &inode) == FALLO) return FALLO;
        if (!INODE_P(inode.permisos, INODE_P_READ)) return ERROR_PERMISO_LECTURA;

        entrada_t entradas[ENTRADAS_IN_BLOCK];
        for (size_t i = 0; i < n; i++) {
            size_t idx = i % ENTRADAS_IN_BLOCK;
            if (idx == 0 && mi_read_f(p_inodo, entradas, i * sizeof(entrada_t), BLOCKSIZE) == FALLO) return FALLO;
            // printf("entradas[%ld].nombre: %s\n", idx, entradas[idx].nombre);
            if (leer_inodo(entradas[idx].ninodo, &inode) == FALLO) return FALLO;

            char rutaFiAct[TAMBUFFER];
            snprintf(rutaFiAct, sizeof(rutaFiAct), "%s%s%s", ruta, ruta[strlen(ruta) - 1] == '/' ? "" : "/", entradas[idx].nombre);

            // Si el directorio contiene otro directorio y este no está vacío
            if (inode.tipo == 'd' && inode.tamEnBytesLog / sizeof(entrada_t) != 0) {
                // printf(RESET "El directorio contiene otro directorio y este no está vacío\n");
                // printf(RESET "Ejecutando mi_rm -r recursivo con\n");
                // printf(RESET "rutaFiAct: %s\n\n", rutaFiAct);

                rtrn = rm_function(rutaFiAct, 1);
                if (rtrn < 0) return rtrn;
                continue;
            }

            // Si es un fichero o si es un directorio y está vacío, llamamos a la propia función en modo normal
            rtrn = rm_function(rutaFiAct, 0);
            if (rtrn < 0) return rtrn;
        }
        rtrn = rm_function(ruta, 0);
        if (rtrn < 0) return rtrn;
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
