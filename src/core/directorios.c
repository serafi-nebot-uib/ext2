/**************************************************************************
* FILENAME: directorios.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include <sys/time.h>

#include "util/helper.h"
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

/**
 *  Extrae, a partir de un string cuyo contenido es la ruta de un archivo o directorio,
 *  el contenido final del camino
 *
 * @param camino puntero al string con la ruta a extraer
 * @param final puntero al string donde se escribirá el trozo final del camino
 * @return EXITO si se extrae el valor correctamente, FALLO en caso contrario
 */
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
    if (sb_read(&sb) == FALLO) return FALLO;

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
    DEBUG(2, "inicial: %s, final: %s, reservar: %d", inicial, final, reservar);

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

    if (strcmp(inicial, entrada.nombre) != 0 && num_entrada_inodo == cant_entradas_inodo) { // la entrada no existe
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
        DEBUG(2, "reservado inodo %u tipo %c con permisos %u para %s", entrada.ninodo, tmp.tipo, tmp.permisos, entrada.nombre);

        // escribir la entrada en el directorio padre por el final
        if (mi_write_f(*p_inodo_dir, &entrada, cant_entradas_inodo * sizeof(entrada_t), sizeof(entrada_t)) == FALLO) {
            if (entrada.ninodo != -1) liberar_inodo(entrada.ninodo);     // si se habia reservado un inodo para la entrada, se libera
            return FALLO;
        }
        DEBUG(2, "creada entrada: %s, %u", entrada.nombre, entrada.ninodo);
    }

    if (strcmp(final, "") == 0 || strcmp(final, "/") == 0) {                                             // si final está vacío o sólo tiene "/", hemos llegado al final del camino
        if (num_entrada_inodo < cant_entradas_inodo && reservar == 1) return ERROR_ENTRADA_YA_EXISTENTE; // modo escritura y la entrada ya existe cortamos la recursividad
        DEBUG(2, "*p_inodo = entrada.ninodo = %u", entrada.ninodo);
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
    if (sb_read(&sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    if (ret != EXITO) return ret;

    DEBUG(2, "camino: %s; permisos: %hhu", camino, permisos);
    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    return EXITO;
}

/**
 * Función auxiliar que añade entradas y formatea el buffer de la función mi_dir
 *
 * @param nombre nombre del fichero / directorio a añadir al buffer
 * @param inodo_t inodo del fichero / directorio a añadir al buffer
 * @param buffer buffer sobre el cual se está trabajando
 * @param flag permite seleccionar el modo de formateo del buffer
 */
void mi_dir_entrada(const char *const nombre, inodo_t *inode, char *buffer, char flag) {
    char tmp[80] = { 0 };
    const char *const color = inode->tipo == 'd' ? BLUE : GREEN;
    if (flag) {
        strncat(buffer, (const char *)&inode->tipo, 1);
        strcat(buffer, "\t");
        strcat(buffer, INODE_P(inode->permisos, INODE_P_READ) ? "r" : "-");
        strcat(buffer, INODE_P(inode->permisos, INODE_P_WRITE) ? "w" : "-");
        strcat(buffer, INODE_P(inode->permisos, INODE_P_EXECUTE) ? "x" : "-");
        strcat(buffer, "\t");
        struct tm *tm;
        tm = localtime(&inode->mtime);
        snprintf(tmp, sizeof(tmp), "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
        strcat(buffer, tmp);
        strcat(buffer, "\t");
        snprintf(tmp, sizeof(tmp), "%u", inode->tamEnBytesLog);
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

/**
 * Función que devuelve un buffer con el contenido de un directorio pasado por parámetro
 *
 * @param camino ruta del directorio a imprimir
 * @param buffer posición de memoria donde se almacena el contenido de un directorio
 * @param flag permite seleccionar el formato de impresión
 * @return Número de entradas del directorio en caso de éxito,
 *         FALLO/código de error en caso contrario.
 */
int mi_dir(const char *camino, char *buffer, char flag) {
    superbloque_t sb;
    if (sb_read(&sb) == FALLO) return FALLO;

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
    size_t n = inode.tamEnBytesLog / sizeof(entrada_t);
    DEBUG(2, "n = inode.tamEnBytesLog / sizeof(entrada_t) = %u / %lu = %zu", inode.tamEnBytesLog, sizeof(entrada_t), n);

    if (inode.tipo == 'd') {
        strcat(buffer, "Total: ");
        char tmp[24] = {0};
        snprintf(tmp, sizeof(tmp), "%lu", n);
        strcat(buffer, tmp);
        strcat(buffer, "\n");
    }

    if (flag && n > 0) {
        strcat(buffer, "Tipo\tModo\tmTime\t\t\tTamaño\tNombre\n");
        memset(line, '-', TAMFILA);
        line[64 - 1] = 0;
        line[64 - 2] = '\n';
        strcat(buffer, line);
    }

    // allow listing single files
    if (inode.tipo != 'd') {
        char final[strlen(camino)];
        if (extraer_camino_final(camino, final) == FALLO) return FALLO;
        mi_dir_entrada(final, &inode, buffer, flag);
        strcat(buffer, "\n" RESET);
        return 1;
    }

    if (n == 0) return 0;

    entrada_t entradas[ENTRADAS_IN_BLOCK];
    for (size_t i = 0; i < n; i++) {
        size_t idx = i % ENTRADAS_IN_BLOCK;
        if (idx == 0 && mi_read_f(p_inodo, entradas, i * sizeof(entrada_t), BLOCKSIZE) == FALLO) return FALLO;
        DEBUG(1, "entrada %zu -> ninodo: %u; nombre: %s", i, entradas[idx].ninodo, entradas[idx].nombre);
        if (leer_inodo(entradas[idx].ninodo, &inode) == FALLO) return FALLO;
        mi_dir_entrada(entradas[idx].nombre, &inode, buffer, flag);
    }
    strcat(buffer, "\n");

    return n;
}

/**
 * Cambia los permisos de un fichero o directorio
 *
 * @param camino ruta del directorio o fichero
 * @param permisos nivel de permisos a establecer (en octal)
 * @return EXITO en caso correcto, FALLO/código de error en caso contrario.
 */
int mi_chmod(const char *camino, unsigned char permisos) {
    superbloque_t sb;
    if (sb_read(&sb) == FALLO) return FALLO;

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

/**
 * Obtiene el inodo asociado a una entrada pasada por parámetro
 * y obtiene sus stats mediante una posterior llamada a mi_stat_f()
 *
 * @param camino ruta del directorio o fichero
 * @param p_stat estructura de datos a la cual volcar la metainformación
 * @return posición del inodo en caso correcto,
 *         FALLO/código de error en caso contrario.
 */
int mi_stat(const char *camino, stat_t *p_stat) {
    superbloque_t sb;
    if (sb_read(&sb) == FALLO) return FALLO;

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

/**
 * Obtener entrada en la cache.
 *
 * @param camino ruta de la entrada a buscar en la cache
 * @param p_inodo puntero a la variable dónde se va a almacenar el número de inodo asociado a camino
 * @return 0 si se ha encontrado la entrada especificada, -1 en caso contrario
 */
int entrada_cache_get(const char *const camino, unsigned int *const p_inodo) {
#if CACHE == 0
    return -1;
#else
    int ret = -1;

#if CACHE == 1
    if (strcmp(entrada_cache.camino, camino) == 0) {
        *p_inodo = entrada_cache.p_inodo;
        ret = 0;
    }
#elif CACHE == 2 || CACHE == 3
    unsigned int idx = 0;
    for (size_t i = 0; i < CACHE_SIZE && ret < 0; i++) {
        if (strcmp(entrada_cache[i].camino, camino) == 0) {
            idx = i;
            *p_inodo = entrada_cache[i].p_inodo;
            ret = 0;
        }
    }
    if (ret == 0) {
        DEBUG(1, CYAN "utilizamos cache[%u]: %s" RESET, idx, camino);
#if CACHE == 3
        gettimeofday(&entrada_cache[idx].ultima_consulta, NULL);
#endif
    }
#endif

    return ret;
#endif
}

/**
 * Insertar entrada en la cache.
 *
 * @param camino ruta de la entrada a insertar en la cache
 * @param p_inodo número de inodo asociado a la ruta especificada
 */
void entrada_cache_put(const char *const camino, unsigned int p_inodo) {
#if CACHE > 0

#if CACHE == 1
    strncpy(entrada_cache.camino, camino, sizeof(entrada_cache.camino));
    entrada_cache.p_inodo = p_inodo;
#else
    unsigned int idx = entrada_cache_top;
#if CACHE == 2
    entrada_cache_top = (entrada_cache_top + 1) % CACHE_SIZE;
#endif
#if CACHE == 3
    for (size_t i = 0; i < CACHE_SIZE; i++) {
        if (!timerisset(&entrada_cache[i].ultima_consulta)) {
            DEBUG(3, "entrada_cache[%zu] timer is not set", i);
            idx = i;
            break;
        }
        if (timercmp(&entrada_cache[i].ultima_consulta, &entrada_cache[idx].ultima_consulta, <)) {
            idx = i;
        }
    }
    gettimeofday(&entrada_cache[idx].ultima_consulta, NULL);
#endif
    entrada_cache_t *cache = &entrada_cache[idx];
    strncpy(cache->camino, camino, sizeof(cache->camino));
    cache->p_inodo = p_inodo;
    DEBUG(1, ORANGE "reemplazamos cache[%u]: %s" RESET, idx, camino);
#endif

#endif
}

/**
 * Escribir n bytes a los datos de un inodo.
 *
 * @param ninodo número de inodo al que escribir
 * @param buf_original buffer de datos origen; de dónde se van a volcar los datos
 * @param offset número de byte del inodo del cual empezar a escribir
 * @param nbytes número de bytes a escribir
 * @return número de bytes escritos, FALLO en caso de error
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;

    if (entrada_cache_get(camino, &p_inodo) < 0) {
        superbloque_t sb;
        if (sb_read(&sb) == FALLO) return FALLO;

        p_inodo_dir = sb.posInodoRaiz;
        int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (ret > 0) {
            mostrar_error_buscar_entrada(ret);
            return ret;
        }

#if CACHE == 1
        DEBUG(1, ORANGE "actualizamos la cache de escritura" RESET);
#endif
        entrada_cache_put(camino, p_inodo);
    } else {
#if CACHE == 1
        DEBUG(1, CYAN "utilizamos la caché de escritura en vez de llamar a buscar_entrada()" RESET);
#endif
    }

    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    return mi_write_f(p_inodo, buf, offset, nbytes);
}

int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;

    if (entrada_cache_get(camino, &p_inodo) < 0) {
        // DEBUG(1, "\"%s\" not found in entrada cache", camino);
        superbloque_t sb;
        if (sb_read(&sb) == FALLO) return FALLO;

        p_inodo_dir = sb.posInodoRaiz;
        int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (ret > 0) {
            mostrar_error_buscar_entrada(ret);
            return ret;
        }

#if CACHE == 1
        DEBUG(1, ORANGE "actualizamos la cache de lectura" RESET);
#endif
        entrada_cache_put(camino, p_inodo);
    } else {
#if CACHE == 1
        DEBUG(1, CYAN "utilizamos la caché de lectura en vez de llamar a buscar_entrada()" RESET);
#endif
    }

    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    return mi_read_f(p_inodo, buf, offset, nbytes);
}

/**
 * Crear enlace de una entrada directorio camino2 al inodo especificado por la entrada de directorio camino1.
 *
 * @param camino1 ruta del fichero original
 * @param camino2 ruta del enlace
 * @return EXITO si se crea el enlace correctamente, código de error en caso contrario
 */
int mi_link(const char *camino1, const char *camino2) {
    superbloque_t sb;
    if (sb_read(&sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir_1 = sb.posInodoRaiz, p_inodo_1 = 0, p_entrada_1 = 0;
    int ret = buscar_entrada(camino1, &p_inodo_dir_1, &p_inodo_1, &p_entrada_1, 0, 0);
    if (ret < 0) {
        mostrar_error_buscar_entrada(ret);
        return ret;
    }

    DEBUG(2, "p_inodo_dir_1: %u", p_inodo_dir_1);
    DEBUG(2, "p_inodo_1: %u", p_inodo_1);
    DEBUG(2, "p_entrada_1: %u", p_entrada_1);

    inodo_t inodo_1;
    if (leer_inodo(p_inodo_1, &inodo_1) == FALLO) return FALLO;
    if (inodo_1.tipo != 'f') return FALLO;
    if (!INODE_P(inodo_1.permisos, INODE_P_READ)) return ERROR_PERMISO_LECTURA;

    unsigned int p_inodo_dir_2 = sb.posInodoRaiz, p_inodo_2 = 0, p_entrada_2 = 0;
    ret = buscar_entrada(camino2, &p_inodo_dir_2, &p_inodo_2, &p_entrada_2, 1, 6);
    if (ret < 0) {
        mostrar_error_buscar_entrada(ret);
        return ret;
    }

    DEBUG(2, "p_inodo_dir_2: %u", p_inodo_dir_2);
    DEBUG(2, "p_inodo_2: %u", p_inodo_2);
    DEBUG(2, "p_entrada_2: %u", p_entrada_2);

    entrada_t entrada_2;
    if (mi_read_f(p_inodo_dir_2, &entrada_2, p_entrada_2 * sizeof(entrada_2), sizeof(entrada_2)) == FALLO) return FALLO;
    entrada_2.ninodo = p_inodo_1;
    if (mi_write_f(p_inodo_dir_2, &entrada_2, p_entrada_2 * sizeof(entrada_2), sizeof(entrada_2)) == FALLO) return FALLO;
    if (liberar_inodo(p_inodo_2) == FALLO) return FALLO;

    inodo_1.nlinks++;
    inodo_1.ctime = time(NULL);
    if (escribir_inodo(p_inodo_1, &inodo_1) == FALLO) return FALLO;

    return EXITO;
}

/**
 * Función que borra la entrada de directorio especificada
 *
 * @param camino ruta del directorio/fichero a borrar
 * @return EXITO si se borra correctamente, código de error en caso contrario
 */
int mi_unlink(const char *camino) {
    superbloque_t sb;
    if (sb_read(&sb) == FALLO) return FALLO;

    unsigned int p_inodo_dir = sb.posInodoRaiz, p_inodo = 0, p_entrada = 0;
    int ret = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (ret < 0) {
        mostrar_error_buscar_entrada(ret);
        return ret;
    }
    //printf("mi_unlink -> input camino: %s\n", camino);
    DEBUG(2, "p_inodo_dir: %u", p_inodo_dir);
    DEBUG(2, "p_inodo: %u", p_inodo);
    DEBUG(2, "p_entrada: %u", p_entrada);

    inodo_t inodo, inodo_dir;
    if (leer_inodo(p_inodo, &inodo) == FALLO) return FALLO;
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        ERROR("el directorio %s no está vacío", camino);
        return FALLO; // Si se trata de un directorio y no está vacío entonces no se puede borrar
    }
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) return ERROR_PERMISO_ESCRITURA;
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) return FALLO;
    int num_entradas = inodo_dir.tamEnBytesLog / sizeof(entrada_t);
    int ult_entrada = num_entradas - 1;

    if (p_entrada != ult_entrada) {
        entrada_t entrada_aux;
        if (mi_read_f(p_inodo_dir, &entrada_aux, ult_entrada * sizeof(entrada_t), sizeof(entrada_t)) == FALLO) return FALLO;
        if (mi_write_f(p_inodo_dir, &entrada_aux, p_entrada * sizeof(entrada_t), sizeof(entrada_t)) == FALLO) return FALLO;
    }
    if (mi_truncar_f(p_inodo_dir, inodo_dir.tamEnBytesLog - sizeof(entrada_t)) == FALLO) return FALLO;

    inodo.nlinks--;
    if (inodo.nlinks == 0) {
        if (liberar_inodo(p_inodo) == FALLO) return FALLO;
    } else {
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) == FALLO) return FALLO;
    }

    return EXITO;
}
