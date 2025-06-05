/**************************************************************************
* FILENAME: verificacion.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "verificacion.h"
#include "core/directorios.h"
#include "core/ficheros.h"
#include "simulacion.h"

#define REG_BUFFER_SIZE 256
#define TIMESTAMP_FMT "%a %d-%m-%Y %H:%M:%S"

// nombre del dispositivo, se utiliza como variable global para poder utilizarla en la función clean_exit
static const char *dev_name = NULL;

/**
 * Limpiar los recursos y parar la ejecución del programa.
 *
 * @param code código de error con el cual parar el programa
 */
void clean_exit(int code) {
    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        exit(FALLO);
    }
    exit(code);
}

/**
 * Format informacion_t into a string buffer.
 *
 * @param info estructura de informacion a formatear
 * @param buff buffer dónde se va a almacenar el información de la estructura
 * @param size longitud del buffer
 */
void format_info(informacion_t *info, char *const buff, size_t size) {
    if (info == NULL || buff == NULL) return;

    // variable temporal para formatear strings y después concatenar al buffer principal
    char tmp[128];

    snprintf(tmp, sizeof(tmp), "\nPID: %d\n", info->pid);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Numero de escrituras: %d\n", info->nEscrituras);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Primera Escritura\t%d\t%d\t", info->PrimeraEscritura.nEscritura + 1, info->PrimeraEscritura.nRegistro);
    strcat(buff, tmp);
    struct tm *t = localtime(&info->PrimeraEscritura.fecha.tv_sec);
    strftime(tmp, sizeof(tmp), TIMESTAMP_FMT, t);
    strcat(buff, tmp);
    snprintf(tmp, sizeof(tmp), ".%06d\n", info->PrimeraEscritura.fecha.tv_usec);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Ultima Escritura\t%d\t%d\t", info->UltimaEscritura.nEscritura + 1, info->UltimaEscritura.nRegistro);
    strcat(buff, tmp);
    t = localtime(&info->UltimaEscritura.fecha.tv_sec);
    strftime(tmp, sizeof(tmp), TIMESTAMP_FMT, t);
    strcat(buff, tmp);
    snprintf(tmp, sizeof(tmp), ".%06d\n", info->UltimaEscritura.fecha.tv_usec);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Menor Posición\t\t%d\t%d\t", info->MenorPosicion.nEscritura + 1, info->MenorPosicion.nRegistro);
    strcat(buff, tmp);
    t = localtime(&info->MenorPosicion.fecha.tv_sec);
    strftime(tmp, sizeof(tmp), TIMESTAMP_FMT, t);
    strcat(buff, tmp);
    snprintf(tmp, sizeof(tmp), ".%06d\n", info->MenorPosicion.fecha.tv_usec);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Mayor Posición\t\t%d\t%d\t", info->MayorPosicion.nEscritura + 1, info->MayorPosicion.nRegistro);
    strcat(buff, tmp);
    t = localtime(&info->MayorPosicion.fecha.tv_sec);
    strftime(tmp, sizeof(tmp), TIMESTAMP_FMT, t);
    strcat(buff, tmp);
    snprintf(tmp, sizeof(tmp), ".%06d\n", info->MayorPosicion.fecha.tv_usec);
    strcat(buff, tmp);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        ERROR("sintaxis: %s <disco> <directorio_simulacion>", argv[0]);
        return FALLO;
    }

    dev_name = argv[1]; // ruta al dispositivo
    const char *const simdir = argv[2]; // ruta al directorio de simulación dentro del dispositivo

    // comprobar que se trata de un directorio
    if (simdir[strlen(simdir) - 1] != '/') {
        ERROR("directorio_simulacion debe terminar con /");
        return FALLO;
    }

    printf("dir_sim: %s\n", simdir);

    // montar el dispositivo para poder acceder a el
    // a partir de aqui si hay un error crítico se debe salir con clean_exit
    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    // primero se obtiene la información del directorio para calcular la cantidad de subdirectorios (procesos que se han ejecutado en paralelo) que contiene
    stat_t st;
    if (mi_stat(simdir, &st) == FALLO) {
        ERROR("no se ha podido obtener la información del directorio %s", simdir);
        clean_exit(FALLO);
    }

    // si la cantidad de procesos no es la esperada se muestra error y se para la ejecución
    // el resto del programa asume que nproc == NUMPROCESOS
    uint32_t nproc = st.tamEnBytesLog / sizeof(entrada_t);
    if (nproc != NUMPROCESOS) {
        ERROR("el numero de procesos encontrados no coincide con NUMPROCESOS (%u != %d)", nproc, NUMPROCESOS);
        clean_exit(FALLO);
    }

    printf("numentradas: %u NUMPROCESOS: %d\n", nproc, NUMPROCESOS);

    off_t info_offset = 0;
    // info_path contiene la ruta al fichero de salida que se va a generar con el reporte de información
    char info_path[128];
    strncpy(info_path, simdir, sizeof(info_path) / sizeof(*info_path));
    strcat(info_path, "informe.txt");
    // crear el archivo de reporte
    if (mi_creat(info_path, 6) == FALLO) {
        ERROR("no se ha podido crear el archivo de informe: %s", info_path);
        clean_exit(FALLO);
    }

    // leer todas las entradas de golpe (esta garantizado que el numero de procesos == NUMPROCESOS)
    entrada_t entradas[NUMPROCESOS];
    if (mi_read(simdir, entradas, 0, sizeof(entradas)) == FALLO) {
        ERROR("no se han podido leer las entradas de %s", simdir);
        clean_exit(FALLO);
    }

    // iteramos sobre cada una de las entradas del directorio de simulación
    for (size_t i = 0; i < NUMPROCESOS; i++) {
        // a partir del nombre del subdirectorio, obtenemos el PID del proceso
        pid_t pid = atoi(&(entradas[i].nombre[11])); // proceso_PIDXXXX -> XXXX a partir del índice 11
        informacion_t info = { .pid = pid };

        // data_path contiene la ruta al fichero de datos (prueba.dat) generado por el proceso actual
        char data_path[128];
        strncpy(data_path, simdir, sizeof(data_path) / sizeof(*data_path));
        strcat(data_path, entradas[i].nombre);
        strcat(data_path, "/prueba.dat");
        if (mi_stat(data_path, &st) == FALLO) {
            ERROR("no se ha podido obtener la información del archivo de datos %s", data_path);
            continue;
        }

        // array de registros donde sa van a ir leyendo todos los registros
        registro_t registros[REG_BUFFER_SIZE];
        for (size_t reg = 0;; reg += REG_BUFFER_SIZE) {
            // elimniar basura del array de registros
            memset(registros, 0, sizeof(registros));
            // leer del archivo de datos todos los registros hasta rellenar el array de registros
            int s = mi_read(data_path, registros, reg * sizeof(registro_t), sizeof(registros));
            // si no hemos leído ningún registro -> no quedan más -> salimos del bucle
            if (s <= 0) break;

            size_t reg_read = s / sizeof(registro_t); // calcular la cantidad de registros leídos
            // iterar sobre los registros leídos
            for (size_t j = 0; j < reg_read; j++) {
                // comprobar que el registro es válido (pid coincide con el del subdirectorio)
                if (registros[j].pid == pid) {
                    // si es la primera escritura inicializamos los campos del estructura info
                    if (info.nEscrituras == 0) {
                        memcpy(&info.PrimeraEscritura, &registros[j], sizeof(registro_t));
                        memcpy(&info.UltimaEscritura, &registros[j], sizeof(registro_t));
                        memcpy(&info.MenorPosicion, &registros[j], sizeof(registro_t));
                        memcpy(&info.MayorPosicion, &registros[j], sizeof(registro_t));
                    } else {
                        // comparamos que escrituras/registros son mayores/menores y actualizar los campos de info si es debido
                        if (registros[j].nEscritura < info.PrimeraEscritura.nEscritura) memcpy(&info.PrimeraEscritura, &registros[j], sizeof(registro_t));
                        if (registros[j].nEscritura > info.UltimaEscritura.nEscritura) memcpy(&info.UltimaEscritura, &registros[j], sizeof(registro_t));
                        if (registros[j].nRegistro < info.MenorPosicion.nRegistro) memcpy(&info.MenorPosicion, &registros[j], sizeof(registro_t));
                        if (registros[j].nRegistro > info.MayorPosicion.nRegistro) memcpy(&info.MayorPosicion, &registros[j], sizeof(registro_t));
                    }
                    info.nEscrituras++; // incrementamos contados de escrituras validadas
                    // printf("info.nEscrituras: %u\n", info.nEscrituras);
                }
            }
        }

        printf("%4zu) %4u escrituras validadas en %s\n", i + 1, info.nEscrituras, data_path);

        // info_buff contiene el texto formateado de info
        char info_buff[512];
        memset(info_buff, 0, sizeof(info_buff));
        format_info(&info, info_buff, sizeof(info_buff));
        // escribir el texto formateado de info al final del fichero de reporte
        size_t size = strlen(info_buff);
        if (mi_write(info_path, info_buff, info_offset, size) == FALLO) {
            ERROR("no se ha podido escribir en %s", info_path);
            clean_exit(FALLO);
        }
        // incrementar el offset del fichero de reporte para que la próxima iteración se escriba al final
        info_offset += size;
    }

    clean_exit(EXITO);
    return EXITO;
}