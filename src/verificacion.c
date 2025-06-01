#include "verificacion.h"
#include "core/directorios.h"
#include "core/ficheros.h"
#include "simulacion.h"

#define REG_BUFFER_SIZE 256

static const char *dev_name = NULL;

void clean_exit(int code) {
    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        exit(FALLO);
    }
    exit(code);
}

void format_info(informacion_t *info, char *const buff, size_t size) {
    if (info == NULL || buff == NULL) return;

    char tmp[128];

    snprintf(tmp, sizeof(tmp), "\nPID: %d\n", info->pid);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Numero de escrituras: %d\n", info->nEscrituras);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Primera Escritura\t%d\t%d\t", info->PrimeraEscritura.nEscritura, info->PrimeraEscritura.nRegistro);
    strcat(buff, tmp);
    struct tm *t = localtime(&info->PrimeraEscritura.fecha);
    strftime(tmp, sizeof(tmp), "%a %d-%m-%Y %H:%M:%S\n", t);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Ultima Escritura\t%d\t%d\t", info->UltimaEscritura.nEscritura, info->UltimaEscritura.nRegistro);
    strcat(buff, tmp);
    t = localtime(&info->UltimaEscritura.fecha);
    strftime(tmp, sizeof(tmp), "%a %d-%m-%Y %H:%M:%S\n", t);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Menor Posición\t\t%d\t%d\t", info->MenorPosicion.nEscritura, info->MenorPosicion.nRegistro);
    strcat(buff, tmp);
    t = localtime(&info->MenorPosicion.fecha);
    strftime(tmp, sizeof(tmp), "%a %d-%m-%Y %H:%M:%S\n", t);
    strcat(buff, tmp);

    snprintf(tmp, sizeof(tmp), "Mayor Posición\t\t%d\t%d\t", info->MayorPosicion.nEscritura, info->MayorPosicion.nRegistro);
    strcat(buff, tmp);
    t = localtime(&info->MayorPosicion.fecha);
    strftime(tmp, sizeof(tmp), "%a %d-%m-%Y %H:%M:%S\n", t);
    strcat(buff, tmp);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        ERROR("sintaxis: %s <disco> <directorio_simulacion>", argv[0]);
        return FALLO;
    }

    dev_name = argv[1];
    const char *const simdir = argv[2];

    if (simdir[strlen(simdir) - 1] != '/') {
        ERROR("directorio_simulacion debe terminar con /");
        return FALLO;
    }

    printf("dir_sim: %s\n", simdir);

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    stat_t st;
    if (mi_stat(simdir, &st) == FALLO) {
        ERROR("no se ha podido obtener la información del directorio %s", simdir);
        clean_exit(FALLO);
    }

    uint32_t nproc = st.tamEnBytesLog / sizeof(entrada_t);
    if (nproc != NUMPROCESOS && nproc != NUMPROCESOS + 1) {
        ERROR("el numero de procesos encontrados no coincide con NUMPROCESOS (%u != %d)", nproc, NUMPROCESOS);
        clean_exit(FALLO);
    }

    printf("numentradas: %u NUMPROCESOS: %d\n", nproc, NUMPROCESOS);

    off_t info_offset = 0;
    char info_path[128];
    strncpy(info_path, simdir, sizeof(info_path) / sizeof(*info_path));
    strcat(info_path, "informe.txt");
    if (mi_creat(info_path, 6) == FALLO) {
        ERROR("no se ha podido crear el archivo de informe: %s", info_path);
        clean_exit(FALLO);
    }

    entrada_t entradas[NUMPROCESOS];
    if (mi_read(simdir, entradas, 0, sizeof(entradas)) == FALLO) {
        ERROR("no se han podido leer las entradas de %s", simdir);
        clean_exit(FALLO);
    }

    for (size_t i = 0; i < NUMPROCESOS; i++) {
        pid_t pid = atoi(&(entradas[i].nombre[11])); // proceso_PIDXXXX -> XXXX a partir del índice 11
        informacion_t info = { .pid = pid };

        char data_path[128];
        strncpy(data_path, simdir, sizeof(data_path) / sizeof(*data_path));
        strcat(data_path, entradas[i].nombre);
        strcat(data_path, "/prueba.dat");

        if (mi_stat(data_path, &st) == FALLO) {
            ERROR("no se ha podido obtener la información del archivo de datos %s", data_path);
            continue;
        }

        size_t reg_cnt = 0;
        registro_t registros[REG_BUFFER_SIZE];
        while (1) {
            memset(registros, 0, sizeof(registros));
            int s = mi_read(data_path, registros, reg_cnt * sizeof(registro_t) * REG_BUFFER_SIZE, sizeof(registros));
            if (s <= 0) break;

            size_t reg_read = s / sizeof(registro_t);
            for (size_t j = 0; j < reg_read; j++) {
                if (registros[j].pid == pid) {
                    if (info.nEscrituras == 0) {
                        memcpy(&info.PrimeraEscritura, &registros[j], sizeof(registro_t));
                        memcpy(&info.UltimaEscritura, &registros[j], sizeof(registro_t));
                        memcpy(&info.MenorPosicion, &registros[j], sizeof(registro_t));
                        memcpy(&info.MayorPosicion, &registros[j], sizeof(registro_t));
                    } else {
                        if (registros[j].nEscritura < info.PrimeraEscritura.nEscritura) memcpy(&info.PrimeraEscritura, &registros[j], sizeof(registro_t));
                        if (registros[j].nEscritura > info.UltimaEscritura.nEscritura) memcpy(&info.UltimaEscritura, &registros[j], sizeof(registro_t));
                        if (registros[j].nRegistro < info.MenorPosicion.nRegistro) memcpy(&info.MenorPosicion, &registros[j], sizeof(registro_t));
                        if (registros[j].nRegistro > info.MayorPosicion.nRegistro) memcpy(&info.MayorPosicion, &registros[j], sizeof(registro_t));
                    }
                    info.nEscrituras++;
                }
            }

            reg_cnt += reg_read;
        }

        printf("%zu) %zu escrituras validadas en %s\n", i, reg_cnt, data_path);

        char info_buff[512];
        memset(info_buff, 0, sizeof(info_buff));
        format_info(&info, info_buff, sizeof(info_buff));
        size_t size = strlen(info_buff);
        if (mi_write(info_path, info_buff, info_offset, size) == FALLO) {
            ERROR("no se ha podido escribir en %s", info_path);
            clean_exit(FALLO);
        }
        info_offset += size;
    }

    clean_exit(EXITO);
    return EXITO;
}
