CC=gcc
CFLAGS=-c -g -Wall -std=gnu17
#LDFLAGS=-pthread

DEBUG ?= 0
DFLAGS = -DDEBUG_LVL=$(DEBUG)


SRC_DIR=src
TEST_DIR=test
BUILD_DIR=build

# Source files from src directory
SOURCES=$(addprefix $(SRC_DIR)/,\
	mi_mkfs.c bloques.c ficheros_basico.c leer_sf.c ficheros.c escribir.c leer.c permitir.c helper.c truncar.c directorios.c)
	# mi_mkdir.c mi_chmod.c mi_ls.c mi_link.c mi_escribir.c mi_cat.c mi_stat.c mi_rm.c semaforo_mutex_posix.c simulacion.c verificacion.c)

# Source files from test directory
TEST_SOURCES=$(wildcard $(TEST_DIR)/*.c)

LIBRARIES=$(addprefix $(BUILD_DIR)/,\
	bloques.o ficheros_basico.o ficheros.o helper.o directorios.o)
	# semaforo_mutex_posix.o)

INCLUDES=$(addprefix $(SRC_DIR)/,\
	bloques.h ficheros_basico.h ficheros.h directorios.h)
	# directorios.h semaforo_mutex_posix.h simulacion.h)

PROGRAMS=$(addprefix $(BUILD_DIR)/,\
	mi_mkfs leer_sf escribir leer permitir truncar)
	# mi_mkdir mi_chmod mi_ls mi_link mi_escribir mi_cat mi_stat mi_rm simulacion verificacion)

# Add test programs (remove .c extension and add build dir prefix)
TEST_PROGRAMS=$(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)

# Object files from both src and test directories
OBJS=$(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS=$(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR) $(OBJS) $(PROGRAMS)

test: $(BUILD_DIR) $(OBJS) $(TEST_OBJS) $(TEST_PROGRAMS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule for src programs
$(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(LIBRARIES) $(INCLUDES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDES)
	$(CC) $(CFLAGS) $(DFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c $(INCLUDES)
	$(CC) $(CFLAGS) $(DFLAGS) -I $(SRC_DIR) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *~ disco* ext*
