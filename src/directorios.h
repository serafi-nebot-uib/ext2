/**************************************************************************
* FILENAME: directorios.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __DIRECTORIOS_H__
#define __DIRECTORIOS_H__

#include "ficheros.h"

#define DELIM "/"
#define TAMNOMBRE 60 //tamaño del nombre de directorio o fichero, en Ext2 = 256
#define ERROR_CAMINO_INCORRECTO (-2)
#define ERROR_PERMISO_LECTURA (-3)
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4)
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5)
#define ERROR_PERMISO_ESCRITURA (-6)
#define ERROR_ENTRADA_YA_EXISTENTE (-7)
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8)

typedef struct  {
  char nombre[TAMNOMBRE]; //Este campo nombre no incluye la ruta (ni el carácter de separación '/'). 
  unsigned int ninodo;
} entrada_t;

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
void mostrar_error_buscar_entrada(int error);

#endif
