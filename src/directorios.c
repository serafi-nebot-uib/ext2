/**************************************************************************
* FILENAME: directorios.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "directorios.h"

/**
 *  Extrae, a partir de un string cuyo contenido es la ruta de un archivo o directorio,
 *  las distintas partes que lo conforman, obteniendo el contenido y tipo inicial de la ruta
 *  y el contenido final restante
 *
 * @param camino puntero al string con la ruta a extraer
 * @param inicial puntero al string donde se escribirá el primer término del camino
 * @param final puntero al string donde se escribirá el trozo restante de camino
 * @param tipo puntero al carácter donde se escribirá de qué tipo es el primer término extraído del camino
 * @return EXITO si se extrae el valor correctamente, FALLO en caso contrario
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    if (camino == NULL || inicial == NULL || final == NULL || tipo == NULL) return FALLO;
    if (camino[0] != '/') return FALLO;
    camino++;

    char tmp[strlen(camino) + 1]; // Se copia camino a un buffer temporal para que no de el warning al usar const char en strtok
    strcpy(tmp, camino);
    char *token = strtok(tmp, DELIM); // Devuelve el token que precede al delimitador

    char *aux = NULL;
    if ((aux = strchr(camino, DELIM[0])) == NULL || token == NULL) { // no ha encontrado ninguna '/', es un fichero
        strcpy(inicial, camino);                                     //*inicial = *camino;
        *tipo = 'f';
        strcpy(final, "");
    } else {                    // ha obtenido un token antes del '/', es un directorio
        strcpy(inicial, token); // inicial = token;
        *tipo = 'd';
        if (aux != NULL) strcpy(final, aux); // devuelve el resto del string restante, con el '/' inclusive
        else strcpy(final, "");
    }

    return EXITO;
};

/**
 * Busca y crea un archivo o directorio dentro del inodo padre
 *
 * @param camino_parcial ruta del archivo o directorio a buscar o crear
 * @param p_inodo_dir número de inodo del directorio padre dentro del array de inodos
 * @param p_inodo número de inodo al que está asociado el nombre de la entrada buscada
 * @param p_entrada número de entrada dentro del inodo *p_inodo_dir que lo contiene
 * @param reservar si vale 1, y este no existe, crea el archivo o directorio;
 *                 si vale 0, solo busca su existencia dentro del sistema
 * @param permisos en caso de que reservar valga 1, el archivo o directorio se creará con los permisos especificados
 * @return valor entero que representa el tipo de salida de la función, error o éxito
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    if (camino_parcial == NULL || p_inodo_dir == NULL || p_inodo == NULL || p_entrada == NULL) return FALLO;

    superbloque_t sb;
    if (bread(posSB, &sb) == FALLO) return FALLO; // lee el superbloque

    if (strcmp(camino_parcial, "/") == 0) { // si el camino sólo contiene la raíz (/)
        *p_inodo = sb.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    entrada_t entrada;
    inodo_t inodo_dir;
    char inicial[sizeof(entrada.nombre) / sizeof(*entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;

    if (extraer_camino(camino_parcial, inicial, final, &tipo) == FALLO) return ERROR_CAMINO_INCORRECTO;
    DEBUG(1, "inicial: %s, final: %s, reservar: %d", inicial, final, reservar);

    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO) return FALLO;
    if (!INODE_P(inodo_dir.permisos, INODE_P_READ)) return ERROR_PERMISO_LECTURA; // comprueba que el inodo tenga permisos de lectura

    entrada_t bloque_entradas[BLOCKSIZE / sizeof(entrada_t)] = {0};

    unsigned int cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(entrada_t); // calcular cantidad de entradas que contiene el inodo
    unsigned int cant_entradas_bloque = BLOCKSIZE / sizeof(entrada_t);
    unsigned int bloque_idx = 0;
    unsigned int entrada_idx;

    unsigned int num_entrada_inodo; // nº de entrada inicial
    for (num_entrada_inodo = 0; num_entrada_inodo < cant_entradas_inodo; num_entrada_inodo++) {
        entrada_idx = num_entrada_inodo % cant_entradas_bloque;
        if (entrada_idx == 0) {
            if (mi_read_f(*p_inodo_dir, bloque_entradas, bloque_idx++ * BLOCKSIZE, BLOCKSIZE) == FALLO) return FALLO;
        }
        if (strcmp(inicial, (entrada = bloque_entradas[entrada_idx]).nombre) == 0) break;
    }

    if (inicial != entrada.nombre && num_entrada_inodo == cant_entradas_inodo) { // la entrada no existe
        switch (reservar) {
        case 0: return ERROR_NO_EXISTE_ENTRADA_CONSULTA; // modo consulta. Como no existe retornamos error
        case 1: {                                        // modo escritura, creamos la entrada en el directorio referenciado por *p_inodo_dir
            // si el directorio raíz es un fichero no permitir escritura
            if (inodo_dir.tipo == 'f') return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            if (!INODE_P(inodo_dir.permisos, INODE_P_WRITE)) { // si es directorio comprobar que tiene permiso de escritura
                return ERROR_PERMISO_ESCRITURA;
            } else {
                strncpy(entrada.nombre, inicial, TAMNOMBRE - 1); // asigna a la entrada, el nombre encontrado en el camino
                if (tipo == 'd') {                               // es un directorio
                    // si se pretende crear el directorio, es decir, no hay nada más allá en el término final
                    // se reserva un inodo como directorio y se enlaza su índice con la entrada creada
                    if (strcmp(final, "/") == 0) {
                        if ((entrada.ninodo = reservar_inodo('d', permisos)) == FALLO) return FALLO;
                    } else { // si no es el final de la ruta y se está intentado acceder a un archivo a través de un directorio que no existe
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                } else { // si es un fichero, se reserva un inodo como fichero y se asigna a la entrada
                    if ((entrada.ninodo = reservar_inodo('f', permisos)) == FALLO) return FALLO;
                }

                inodo_t tmp;
                if (leer_inodo(entrada.ninodo, &tmp) == FALLO) return FALLO;
                DEBUG(1, "reservado inodo %u tipo %c con permisos %u para %s", entrada.ninodo, tmp.tipo, tmp.permisos, entrada.nombre);

                // escribir la entrada en el directorio padre //por el final
                if (mi_write_f(*p_inodo_dir, &entrada, cant_entradas_inodo * sizeof(entrada_t), sizeof(entrada_t)) == FALLO) {
                    if (entrada.ninodo != -1) liberar_inodo(entrada.ninodo); // si se habia reservado un inodo para la entrada, se libera
                    return FALLO;
                }
                DEBUG(1, "creada entrada: %s, %u", entrada.nombre, entrada.ninodo);
            }
        }
        }
    }

    if (strcmp(final, "") == 0 || strcmp(final, "/") == 0) {                                             // si final está vacío o sólo tiene "/", hemos llegado al final del camino
        if (num_entrada_inodo < cant_entradas_inodo && reservar == 1) return ERROR_ENTRADA_YA_EXISTENTE; // modo escritura y la entrada ya existe
        // cortamos la recursividad
        *p_inodo = entrada.ninodo;      // asigna a *p_inodo el número de inodo del directorio o fichero creado o leido
        *p_entrada = num_entrada_inodo; // asigna a *p_entrada el número de su entrada dentro del último directorio que lo contiene
        return EXITO;
    } else {                           // si no se ha llegado al final, se asigna el ninodo de la entrada leída/creada como el ninodo padre y se vuelve a llamar a la función
        *p_inodo_dir = entrada.ninodo; // asigna a *p_inodo_dir el puntero al inodo que se indica en la entrada encontrada;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos); // se vuelve a llamar a la función con el inodo
    }
    return EXITO;
};

/**
 * Imprime en consola el mensaje correspondiente al error pasado por parámetro
 *
 * @param error valor entero correspondiente al error en cuestión
 */
void mostrar_error_buscar_entrada(int error) {
    // fprintf(stderr, "Error: %d\n", error);
    switch (error) {
    case ERROR_CAMINO_INCORRECTO: fprintf(stderr, RED "Error: Camino incorrecto.\n" RESET); break;
    case ERROR_PERMISO_LECTURA: fprintf(stderr, RED "Error: Permiso denegado de lectura.\n" RESET); break;
    case ERROR_NO_EXISTE_ENTRADA_CONSULTA: fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET); break;
    case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO: fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n" RESET); break;
    case ERROR_PERMISO_ESCRITURA: fprintf(stderr, RED "Error: Permiso denegado de escritura.\n" RESET); break;
    case ERROR_ENTRADA_YA_EXISTENTE: fprintf(stderr, RED "Error: El archivo ya existe.\n" RESET); break;
    case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO: fprintf(stderr, RED "Error: No es un directorio.\n" RESET); break;
    }
}

/**
 * Crea un archivo o directorio y sus directorios padres
 *
 * @param camino ruta al archivo/directorio a crear
 * @param permisos permisos con los que crear el archivo/directorio
 * @return valor entero que representa el tipo de salida de la función, error o éxito
*/
int mi_creat(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    return buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
}
