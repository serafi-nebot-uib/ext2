/**************************************************************************
* FILENAME: directorios.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "directorios.h"
#include "ficheros_basico.h"

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
}

int extraer_camino_final(const char *camino, char *const final) {
    if (camino == NULL || final == NULL) return FALLO;

    char tmp[strlen(camino) + 1]; // se copia camino a un buffer temporal para que no de el warning al usar const char en strtok
    strcpy(tmp, camino);
    char *last = NULL;
    char *token = strtok(tmp, DELIM); // devuelve el token que precede al delimitador

    while (token != NULL) {
        last = token;
        token = strtok(NULL, DELIM);
    }

    if (last == NULL) return FALLO;

    strcpy(final, last);

    return EXITO;
}

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

    entrada_t bloque_entradas[BLOCKSIZE / sizeof(entrada_t)] = { 0 };

    unsigned int cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(entrada_t); // calcular cantidad de entradas que contiene el inodo
    unsigned int cant_entradas_bloque = BLOCKSIZE / sizeof(entrada_t);
    unsigned int bloque_idx = 0;
    unsigned int entrada_idx;

    unsigned int num_entrada_inodo = 0; // nº de entrada inicial
    for (; num_entrada_inodo < cant_entradas_inodo; num_entrada_inodo++) {
        entrada_idx = num_entrada_inodo % cant_entradas_bloque;
        if (entrada_idx == 0)
            if (mi_read_f(*p_inodo_dir, bloque_entradas, bloque_idx++ * BLOCKSIZE, BLOCKSIZE) == FALLO) return FALLO;
        if (strcmp(inicial, (entrada = bloque_entradas[entrada_idx]).nombre) == 0) break;
    }

    if (inicial != entrada.nombre && num_entrada_inodo == cant_entradas_inodo) { // la entrada no existe
        if (reservar == 0) return ERROR_NO_EXISTE_ENTRADA_CONSULTA; // modo consulta. Como no existe retornamos error

        // modo escritura, creamos la entrada en el directorio referenciado por *p_inodo_dir
        // si el directorio raíz es un fichero no permitir escritura
        if (inodo_dir.tipo == 'f') return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;

        // si es directorio comprobar que tiene permiso de escritura
        if (!INODE_P(inodo_dir.permisos, INODE_P_WRITE)) return ERROR_PERMISO_ESCRITURA;

        strncpy(entrada.nombre, inicial, TAMNOMBRE - 1);     // asigna a la entrada, el nombre encontrado en el camino
        if (tipo == 'd') {                                   // es un directorio
            // si se pretende crear el directorio, es decir, no hay nada más allá en el término final
            // se reserva un inodo como directorio y se enlaza su índice con la entrada creada
            if (strcmp(final, "/") != 0) return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO; // si no es el final de la ruta y se está intentado acceder a un archivo a través de un directorio que no existe
        }
        if ((entrada.ninodo = reservar_inodo(tipo, permisos)) == FALLO) return FALLO;

        inodo_t tmp;
        if (leer_inodo(entrada.ninodo, &tmp) == FALLO) return FALLO;
        DEBUG(1, "reservado inodo %u tipo %c con permisos %u para %s", entrada.ninodo, tmp.tipo, tmp.permisos, entrada.nombre);

        // escribir la entrada en el directorio padre por el final
        if (mi_write_f(*p_inodo_dir, &entrada, cant_entradas_inodo * sizeof(entrada_t), sizeof(entrada_t)) == FALLO) {
            if (entrada.ninodo != -1) liberar_inodo(entrada.ninodo);     // si se habia reservado un inodo para la entrada, se libera
            return FALLO;
        }
        DEBUG(1, "creada entrada: %s, %u", entrada.nombre, entrada.ninodo);
    }

    if (strcmp(final, "") == 0 || strcmp(final, "/") == 0) {                                             // si final está vacío o sólo tiene "/", hemos llegado al final del camino
        if (num_entrada_inodo < cant_entradas_inodo && reservar == 1) return ERROR_ENTRADA_YA_EXISTENTE; // modo escritura y la entrada ya existe cortamos la recursividad
        *p_inodo = entrada.ninodo;      // asigna a *p_inodo el número de inodo del directorio o fichero creado o leido
        *p_entrada = num_entrada_inodo; // asigna a *p_entrada el número de su entrada dentro del último directorio que lo contiene
        return EXITO;
    } else {                           // si no se ha llegado al final, se asigna el ninodo de la entrada leída/creada como el ninodo padre y se vuelve a llamar a la función
        *p_inodo_dir = entrada.ninodo; // asigna a *p_inodo_dir el puntero al inodo que se indica en la entrada encontrada;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos); // se vuelve a llamar a la función con el inodo
    }

    return EXITO;
}

/**
 * Imprime en consola el mensaje correspondiente al error pasado por parámetro
 *
 * @param error valor entero correspondiente al error en cuestión
 */
void mostrar_error_buscar_entrada(int error) {
    // fprintf(stderr, "Error: %d\n", error);
    switch (error) {
    case ERROR_CAMINO_INCORRECTO:                       ERROR("camino incorrecto"); break;
    case ERROR_PERMISO_LECTURA:                         ERROR("permiso denegado de lectura"); break;
    case ERROR_NO_EXISTE_ENTRADA_CONSULTA:              ERROR("no existe el archivo o el directorio"); break;
    case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO:         ERROR("no existe algún directorio intermedio"); break;
    case ERROR_PERMISO_ESCRITURA:                       ERROR("permiso denegado de escritura"); break;
    case ERROR_ENTRADA_YA_EXISTENTE:                    ERROR("el archivo ya existe"); break;
    case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO: ERROR("no es un directorio"); break;
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
    superbloque_t sb;
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    if (ret != EXITO) return ret;

    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    inodo_t inodo_dir, inodo;
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO || leer_inodo(p_inodo, &inodo)) return FALLO;
    if (!INODE_P(inodo_dir.permisos, INODE_P_WRITE)) return ERROR_PERMISO_ESCRITURA;
    if (!INODE_P(inodo_dir.permisos, INODE_P_READ)) return ERROR_PERMISO_LECTURA; // necessary?
    if (inodo_dir.tipo != 'd') return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;

    return EXITO;
}

void mi_dir_entrada(const char *const nombre, inodo_t *inode, char *buffer, char flag) {
    char tmp[24] = { 0 };
    const char *const color = inode->tipo == 'd' ? BLUE : GREEN;
    if (flag) {
        strncat(buffer, (const char *)&inode->tipo, 1);
        strcat(buffer, "\t");
        strcat(buffer, INODE_P(inode->permisos, INODE_P_READ) ? "r" : "-");
        strcat(buffer, INODE_P(inode->permisos, INODE_P_WRITE) ? "w" : "-");
        strcat(buffer, INODE_P(inode->permisos, INODE_P_EXECUTE) ? "x" : "-");
        strcat(buffer, "\t\t");
        struct tm *tm;
        tm = localtime(&inode->mtime);
        sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
        strcat(buffer, tmp);
        strcat(buffer, "\t");
        sprintf(tmp, "%u", inode->tamEnBytesLog);
        strcat(buffer, tmp);
        strcat(buffer, "\t");
        strcat(buffer, color);
        strcat(buffer, nombre);
        strcat(buffer, RESET);
        strcat(buffer, "\n");
    } else {
        strcat(buffer, color);
        strcat(buffer, nombre);
        strcat(buffer, "\t" RESET);
    }
}

int mi_dir(const char *camino, char *buffer, char flag) {
    superbloque_t sb;
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (ret < 0) return ret;

    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    inodo_t inode;
    if (leer_inodo(p_inodo, &inode) == FALLO) return FALLO;
    if (!INODE_P(inode.permisos, INODE_P_READ)) return ERROR_PERMISO_LECTURA;

    char line[TAMFILA] = { 0 };

    if (flag) {
        strcat(buffer, "tipo\tpermisos\tmtime\t\t\ttamaño\tnombre\n");
        memset(line, '-', TAMFILA);
        line[64 - 1] = 0;
        line[64 - 2] = '\n';
        strcat(buffer, line);
    }

    // allow listing single files
    if (inode.tipo != 'd') {
        char inicial[TAMNOMBRE];
        char final[strlen(camino)];
        if (extraer_camino_final(camino, final) == FALLO) return FALLO;
        mi_dir_entrada(final, &inode, buffer, flag);
        return 1;
    }

    size_t n = inode.tamEnBytesLog / sizeof(entrada_t);
    entrada_t entradas[ENTRADAS_IN_BLOCK];
    if (mi_read_f(p_inodo, entradas, 0, BLOCKSIZE) == FALLO) return FALLO;
    for (size_t i = 0; i < n; i++) {
        DEBUG(1, "entrada %zu -> ninodo: %u; nombre: %s", i, entradas[i].ninodo, entradas[i].nombre);
        if (leer_inodo(entradas[i].ninodo, &inode) == FALLO) return FALLO;
        mi_dir_entrada(entradas[i].nombre, &inode, buffer, flag);
    }
    strcat(buffer, "\n");

    return n;
}

int mi_chmod(const char *camino, unsigned char permisos) {
    superbloque_t sb;
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (ret > 0) {
        mostrar_error_buscar_entrada(ret);
        return ret;
    }

    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    return mi_chmod_f(p_inodo, permisos);
}

int mi_stat(const char *camino, stat_t *p_stat) {
    superbloque_t sb;
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (ret > 0) {
        mostrar_error_buscar_entrada(ret);
        return ret;
    }

    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    return mi_stat_f(p_inodo, p_stat) == EXITO ? p_inodo : FALLO;
}
