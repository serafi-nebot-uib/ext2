CC=gcc
CFLAGS=-c -g -Wall -std=gnu17
#LDFLAGS=-pthread

SRC_DIR=src
BUILD_DIR=build

SOURCES=$(addprefix $(SRC_DIR)/,\
	mi_mkfs.c bloques.c)
	#ficheros_basico.c leer_sf.c ficheros.c escribir.c leer.c truncar.c permitir.c directorios.c mi_mkdir.c mi_chmod.c mi_ls.c mi_link.c mi_escribir.c mi_cat.c mi_stat.c mi_rm.c semaforo_mutex_posix.c simulacion.c verificacion.c)

LIBRARIES=$(addprefix $(BUILD_DIR)/,\
	bloques.o)
	#ficheros_basico.o ficheros.o directorios.o semaforo_mutex_posix.o)

INCLUDES=$(addprefix $(SRC_DIR)/,\
	bloques.h)
	#ficheros_basico.h ficheros.h directorios.h semaforo_mutex_posix.h simulacion.h)

PROGRAMS=$(addprefix $(BUILD_DIR)/,\
	mi_mkfs)
	#leer_sf escribir leer truncar permitir mi_mkdir mi_chmod mi_ls mi_link mi_escribir mi_cat mi_stat mi_rm simulacion verificacion)

OBJS=$(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR) $(OBJS) $(PROGRAMS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *~ disco* ext*
